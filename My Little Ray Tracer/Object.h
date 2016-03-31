#pragma once
#include "geometry.h"
#include "math.h"
#include <random>

extern std::random_device rd;
extern std::mt19937 gen;
extern std::uniform_real_distribution<> uniformDis;
extern const double M_PI;

class Object
{
public:
	// CONSTRUCTORS
	Object() : color((float)uniformDis(gen), (float)uniformDis(gen), (float)uniformDis(gen)) {}
	virtual ~Object() {}

	// METHODS
	// Method to compute the intersection of the object with a ray
	// Returns true if an intersection was found, false otherwise
	virtual bool intersect(const Vec3f &, const Vec3f &, float &) const = 0;
	// Method to compute the surface data such as normal and texture coordnates 
	// at the intersection point.
	virtual void getSurfaceData(const Vec3f &, Vec3f &, Vec2f &) const = 0;

	// DATA
	Vec3f color;
};

class Sphere : public Object
{
public:
	// CONSTRUCTORS
	Sphere(const Vec3f &c, const float &r) : radius(r), radius2(r *r), center(c) {}

	// METHODS
	// Ray-sphere intersection test
	// \param orig is the ray origin
	// \param dir is the ray direction
	// \param[out] t is the distance from orig to the closest intersection point
	bool intersect(const Vec3f &orig, const Vec3f &dir, float &t) const
	{
		float t0, t1; // solutions for t if the ray intersects
#if 0
					  // geometric solution
		Vec3f L = center - orig;
		float tca = L.dotProduct(dir);
		if (tca < 0) return false;
		float d2 = L.dotProduct(L) - tca * tca;
		if (d2 > radius2) return false;
		float thc = sqrt(radius2 - d2);
		t0 = tca - thc;
		t1 = tca + thc;
#else
					  // analytic solution
		Vec3f L = orig - center;
		float a = dir.dotProduct(dir);
		float b = 2 * dir.dotProduct(L);
		float c = L.dotProduct(L) - radius2;
		if (!solveQuadratic(a, b, c, t0, t1)) return false;
#endif
		if (t0 > t1) std::swap(t0, t1);

		if (t0 < 0) {
			t0 = t1; // if t0 is negative, let's use t1 instead
			if (t0 < 0) return false; // both t0 and t1 are negative
		}

		t = t0;

		return true;
	}
	// Set surface data such as normal and texture coordinates at a given point on the surface
	// \param Phit is the point ont the surface we want to get data on
	// \param[out] Nhit is the normal at Phit
	// \param[out] tex are the texture coordinates at Phit
	void getSurfaceData(const Vec3f &Phit, Vec3f &Nhit, Vec2f &tex) const
	{
		Nhit = Phit - center;
		Nhit.normalize();
		// In this particular case, the normal is similar to a point on a unit sphere
		// centred around the origin. We can thus use the normal coordinates to compute
		// the spherical coordinates of Phit.
		// atan2 returns a value in the range [-pi, pi] and we need to remap it to range [0, 1]
		// acosf returns a value in the range [0, pi] and we also need to remap it to the range [0, 1]
		tex.x = (1.0f + atan2(Nhit.z, Nhit.x) / (float)M_PI) * 0.5f;
		tex.y = acosf(Nhit.y) / (float)M_PI;
	}

	// DATA
	float radius, radius2;
	Vec3f center;
};