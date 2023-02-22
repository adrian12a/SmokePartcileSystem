#include "Particle.hpp"
#include <iostream>

Particle::Particle() {
	setStartingPoint(0.0f, 0.0f, 0.0f);
	active = false;
}

void Particle::activate() {
	active = true;
	life = 1.2f; // Pełnia życia (dla 0 cząstka ginie)
	fade = 0.05f + getRandom() * 0.01f; // Tempo umierania (1/sekunda)
	// Początkowa pozycja cząstki (emiter punktowy):
	setStartingPoint(getRandom() / 200, 0.0, getRandom() / 200);
	// Przeliczenie na układ kartezjański:
	float fi = 3.14 / 4; // 45 stopni w górę
	float psi = getRandom() * (3.14 * 2); // 0-360 stopni wokół osi Y
	float rr = getRandom() + 1; // Długość wektora ruchu
	xi = rr * cos(fi) * cos(psi) * getRandom() / 100 * 0.2;
	yi = rr * sin(fi);
	zi = rr * cos(fi) * sin(psi) * getRandom() / 100 * 0.2;
	// Wektor grawitacji:
	xg = zg = 0;
	yg = 10;
}

void Particle::setStartingPoint(float x, float y, float z) {
	this->x = x;
	this->y = y;
	this->z = z;
}

float Particle::getRandom() {
	return (float)rand() / 100;
}

void Particle::live(float t) {
	x += xi * t;
	y += yi * t;
	z += zi * t;
	xi += xg * t;
	yi += yg * t;
	zi += zg * t;
	// Zabranie życia:
	life -= fade * t * 20;
	//Jeżeli cząstka "umarła", reaktywacja:
	if (life <= 0.0f)
		activate();
}