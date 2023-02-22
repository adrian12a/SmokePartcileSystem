#pragma once
class Particle {
public:
	Particle();
	void activate();
	void setStartingPoint(float x, float y, float z);
	void live(float t);
	float getRandom();
	bool active; // Cząstka aktywna?
	float life, fade; // Czas życia i tempo "umierania"
	float x, y, z; // Aktualna pozycja X-Y-Z
	float xi, yi, zi; // Wektor ruchu X-Y-Z
	float xg, yg, zg; // Wektor grawitacji X-Y-Z
};