#include "scurve_planner.h"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <iostream>

SCurvePlanner::SCurvePlanner() {
}

void SCurvePlanner::setFeedOverride(double override_factor) {
    feed_override = override_factor;
}

std::vector<SPI_Poly_Segment_t> SCurvePlanner::PlanLinearMove(double dx, double dz, double dy1, double dy2, double feedrate_mm_min) {
    std::vector<SPI_Poly_Segment_t> segments;
    
    // vektorlänge
    double D = std::sqrt(dx*dx + dz*dz + dy1*dy1);
    if (D <= 0.001) return segments;

    // normierte vektoren
    double dir_x = dx / D;
    double dir_z = dz / D;
    double dir_y1 = dy1 / D;
    double dir_y2 = dy2 / D;

    const double MaxJerk  = 5000.0;
    const double MaxAccel = 500.0;
    double target_V = (feedrate_mm_min / 60.0) * feed_override; 
    double path_p = 0.0;
    double path_v = 0.0;
    double path_a = 0.0;
    double dt = SegmentDurationS;

    // iterate until destination
    int max_iters = static_cast<int>((D / (target_V > 0.01 ? target_V : 1.0)) / dt) + 500;
    int iter = 0;
    while ((path_p < D || path_v > 0.01) && ++iter < max_iters) {
        
        double path_j = 0.0;
        double stopping_distance = (path_v * path_v) / (2.0 * MaxAccel) + (path_v * (MaxAccel / MaxJerk));

        // increase/decrease velocity, acceleration, jerk
        if (D - path_p <= stopping_distance) {
            if (path_a > -MaxAccel) path_j = -MaxJerk;
            else path_j = 0;
        } 
        else if (path_v < target_V) {
            if (path_a < MaxAccel) path_j = MaxJerk;
            else path_j = 0;
        } 
        else {
            if (path_a > 0) path_j = -MaxJerk;
            else path_j = 0;
        }

        // safety
        if (path_a + path_j * dt > MaxAccel)  path_j = (MaxAccel - path_a) / dt;
        if (path_a + path_j * dt < -MaxAccel) path_j = (-MaxAccel - path_a) / dt;

        // create SPI polysegment packed
        SPI_Poly_Segment_t msg;
        std::memset(&msg, 0, sizeof(SPI_Poly_Segment_t));
        msg.total_ticks = static_cast<uint32_t>(dt * TimerFreqHZ);
        msg.flags = CNC_CMD_SEGMENT;

        if (dx >= 0) msg.directions |= (1 << 0);
        if (dz >= 0) msg.directions |= (1 << 1);
        if (dy1 >= 0) msg.directions |= (1 << 2);
        if (dy2 >= 0) msg.directions |= (1 << 3);

        // convert mm/s to steps per tick (100kHz)
        msg.x_v0 = (path_v * dir_x) * StepsPerMM / TimerFreqHZ;
        msg.x_a0 = (path_a * dir_x) * StepsPerMM / (TimerFreqHZ * TimerFreqHZ);
        msg.x_j  = (path_j * dir_x) * StepsPerMM / (TimerFreqHZ * TimerFreqHZ * TimerFreqHZ);

        msg.z_v0 = (path_v * dir_z) * StepsPerMM / TimerFreqHZ;
        msg.z_a0 = (path_a * dir_z) * StepsPerMM / (TimerFreqHZ * TimerFreqHZ);
        msg.z_j  = (path_j * dir_z) * StepsPerMM / (TimerFreqHZ * TimerFreqHZ * TimerFreqHZ);

        msg.y1_v0 = (path_v * dir_y1) * StepsPerMM / TimerFreqHZ;
        msg.y1_a0 = (path_a * dir_y1) * StepsPerMM / (TimerFreqHZ * TimerFreqHZ);
        msg.y1_j  = (path_j * dir_y1) * StepsPerMM / (TimerFreqHZ * TimerFreqHZ * TimerFreqHZ);

        msg.y2_v0 = msg.y1_v0; msg.y2_a0 = msg.y1_a0; msg.y2_j = msg.y1_j;

        segments.push_back(msg);

        // update values (for taylor polynom integration)
        path_p += (path_v * dt) + (0.5 * path_a * dt * dt) + ((1.0/6.0) * path_j * dt * dt * dt);
        path_v += (path_a * dt) + (0.5 * path_j * dt * dt);
        path_a += (path_j * dt);

        // protection for hardware error overflow
        if (path_v < 0) path_v = 0;
        if (path_p >= D && path_v < 0.01) break;
    }

    std::cout << "Strecke: " << D << " mm. Generierte SPI-Segmente: " << segments.size() 
              << " (" << (segments.size() * 20) << " ms Gesamtzeit)" << std::endl;

    return segments;
}

std::vector<SPI_Poly_Segment_t> SCurvePlanner::PlanArcMove(
    double currentX, double currentY, double currentZ,
    double targetX, double targetY, 
    double offsetI, double offsetJ, 
    bool isClockwise, double feedrate_mm_min) 
{
    std::vector<SPI_Poly_Segment_t> all_segments;

    // calculation of arc move
    double centerX = currentX + offsetI;
    double centerY = currentY + offsetJ;
    double radius = std::sqrt(offsetI * offsetI + offsetJ * offsetJ);

    if (radius < 0.001) return all_segments;

    double startAngle = std::atan2(currentY - centerY, currentX - centerX);
    double endAngle = std::atan2(targetY - centerY, targetX - centerX);
    double angularTravel = endAngle - startAngle;

    if (isClockwise) { // G2
        if (angularTravel >= 0) angularTravel -= 2.0 * M_PI;
    } else {           // G3
        if (angularTravel <= 0) angularTravel += 2.0 * M_PI;
    }

    // linearisation of arc move
    double tolerance = 0.05;
    double maxAngleStep = 2.0 * std::acos(1.0 - (tolerance / radius));
    int numSegments = std::ceil(std::abs(angularTravel) / maxAngleStep);

    if (numSegments < 1) numSegments = 1;

    double angleStep = angularTravel / numSegments;
    double zStep = (currentZ - currentZ) / numSegments;

    double lastX = currentX;
    double lastY = currentY;
    double lastZ = currentZ;

    for (int i = 1; i <= numSegments; ++i) {

        double currentStepAngle = startAngle + (i * angleStep);
        double segTargetX = centerX + radius * std::cos(currentStepAngle);
        double segTargetY = centerY + radius * std::sin(currentStepAngle);
        double segTargetZ = lastZ + zStep;
        double dx = segTargetX - lastX;
        double dy = segTargetY - lastY;
        double dz = segTargetZ - lastZ;

        std::vector<SPI_Poly_Segment_t> micro_segments = PlanLinearMove(dx, dz, dy, dy, feedrate_mm_min);
        all_segments.insert(all_segments.end(), micro_segments.begin(), micro_segments.end());

        lastX = segTargetX;
        lastY = segTargetY;
        lastZ = segTargetZ;
    }

    return all_segments;
}