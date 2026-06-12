; === VORSCHUB & KINEMATIK TEST ===
G90 ; Absolute Koordinaten

; 1. Anfahren im Eilgang (Rot, gestrichelt) 
; Nutzt intern unseren harten G0-Speed (5000 mm/min)
G0 X20 Y20 Z0

; 2. Schneller Schruppschnitt (Zügig)
; Die Bewegung sollte flüssig und relativ schnell sein
G1 X120 Y20 F2000

; 3. Extrem langsamer Schlicht-Schnitt
; Hier muss der rote Punkt deutlich spürbar abbremsen und "kriechen"
G1 X120 Y80 F200

; 4. Mittlerer Vorschub (Normal)
; Der Punkt beschleunigt wieder auf eine gut sichtbare Arbeitsgeschwindigkeit
G1 X20 Y80 F800

; 5. Rückzug auf den Nullpunkt im Eilgang
; Der Punkt rast zurück zum Start
G0 X0 Y0