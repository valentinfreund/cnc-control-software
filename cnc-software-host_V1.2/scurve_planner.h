#ifndef SCURVE_PLANNER_H
#define SCURVE_PLANNER_H

#include "cnc_host_protocol.h"
#include <vector>

#define M_PI 3.14159265358979323846

class SCurvePlanner {
public:
    SCurvePlanner();
    
    std::vector<SPI_Poly_Segment_t> PlanLinearMove(double dx, double dz, double dy1, double dy2, double feedrate_mm_min);
    
    void setFeedOverride(double override_factor);

    std::vector<SPI_Poly_Segment_t> PlanArcMove(double currentX, double currentY, double currentZ,
                                                double targetX, double targetY, 
                                                double offsetI, double offsetJ, 
                                                bool isClockwise, double feedrate_mm_min);

private:
    double feed_override = 1.0;
    const double TimerFreqHZ = 100000.0;
    const double SegmentDurationS = 0.020;
    const double StepsPerMM = 400.0;
};

#endif SCURVE_PLANNER_H