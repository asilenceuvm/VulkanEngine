#pragma once

#include <array>
#include <vector>
#include "glm/glm.hpp"

struct CollisionInfo {
	bool colliding;
	glm::vec3 collisionCentroid;
};

struct Collider {
	virtual glm::vec3 FindFurthestPoint(glm::vec3  direction) const = 0;
};

struct MeshCollider : Collider
{
private:
	std::vector<glm::vec3> m_vertices;

public:
	void SetVertices(std::vector<glm::vec3> vertices) {
		m_vertices = vertices;
	}

	std::vector<glm::vec3> GetVertices() {
		return m_vertices;
	}

	glm::vec3 FindFurthestPoint(glm::vec3 direction) const override
	{
		glm::vec3 maxPoint;
		float maxDistance = -FLT_MAX;

		for (glm::vec3 vertex : m_vertices) {
			float distance = glm::dot(vertex, direction);
			if (distance > maxDistance) {
				maxDistance = distance;
				maxPoint = vertex;
			}
		}
		return maxPoint;
	}
};

inline glm::vec3 Support(const Collider* colliderA, const Collider* colliderB, glm::vec3 direction)
{
	return colliderA->FindFurthestPoint(direction) - colliderB->FindFurthestPoint(-direction);
}

struct Simplex {
private:
	std::array<glm::vec3, 4> m_points;
	unsigned m_size;

public:
	Simplex() : m_points({ glm::vec3{0,0,0}, glm::vec3{0,0,0}, glm::vec3{0,0,0}, glm::vec3{0,0,0} }), m_size(0) {}

	Simplex& operator=(std::initializer_list<glm::vec3> list) {
		for (auto v = list.begin(); v != list.end(); v++) {
			m_points[std::distance(list.begin(), v)] = *v;
		}
		m_size = list.size();

		return *this;
	}

	void push_front(glm::vec3 point) {
		m_points = { point, m_points[0], m_points[1], m_points[2] };
		m_size = std::min(m_size + 1, 4u);
	}

	glm::vec3& operator[](unsigned i) { return m_points[i]; }
	unsigned size() const { return m_size; }

	auto begin() const { return m_points.begin(); }
	auto end()   const { return m_points.end() - (4 - m_size); }
};

inline bool SameDirection(const glm::vec3& direction, const glm::vec3& ao) {
	return glm::dot(direction, ao) > 0;
}

inline CollisionInfo Line(Simplex& points, glm::vec3& direction) {
	glm::vec3 a = points[0];
	glm::vec3 b = points[1];

	glm::vec3 ab = b - a;
	glm::vec3 ao = -a;

	if (SameDirection(ab, ao)) {
		direction = glm::cross(glm::cross(ab, ao), ab);
	}

	else {
		points = { a };
		direction = ao;
	}

	CollisionInfo info;
	info.colliding = false;
	return info;
}

inline CollisionInfo Triangle(Simplex& points, glm::vec3& direction) {
	glm::vec3 a = points[0];
	glm::vec3 b = points[1];
	glm::vec3 c = points[2];

	glm::vec3 ab = b - a;
	glm::vec3 ac = c - a;
	glm::vec3 ao = -a;

	glm::vec3 abc = glm::cross(ab, ac);

	if (SameDirection(glm::cross(abc, ac), ao)) {
		if (SameDirection(ac, ao)) {
			points = { a, c };
			direction = glm::cross(glm::cross(ac, ao), ac);
		}

		else {
			return Line(points = { a, b }, direction);
		}
	}

	else {
		if (SameDirection(glm::cross(ab, abc), ao)) {
			return Line(points = { a, b }, direction);
		}

		else {
			if (SameDirection(abc, ao)) {
				direction = abc;
			}

			else {
				points = { a, c, b };
				direction = -abc;
			}
		}
	}

	CollisionInfo info;
	info.colliding = false;
	return info;
}

inline CollisionInfo Tetrahedron(Simplex& points, glm::vec3& direction)
{
	glm::vec3 a = points[0];
	glm::vec3 b = points[1];
	glm::vec3 c = points[2];
	glm::vec3 d = points[3];

	glm::vec3 ab = b - a;
	glm::vec3 ac = c - a;
	glm::vec3 ad = d - a;
	glm::vec3 ao = -a;

	glm::vec3 abc = glm::cross(ab, ac);
	glm::vec3 acd = glm::cross(ac, ad);
	glm::vec3 adb = glm::cross(ad, ab);

	if (SameDirection(abc, ao)) {
		return Triangle(points = { a, b, c }, direction);
	}

	if (SameDirection(acd, ao)) {
		return Triangle(points = { a, c, d }, direction);
	}

	if (SameDirection(adb, ao)) {
		return Triangle(points = { a, d, b }, direction);
	}

	CollisionInfo info;
	info.colliding = true;
	info.collisionCentroid = glm::vec3{ (a.x + b.x + c.x + d.x) / 4.0f, (a.y + b.y + c.y + d.y) / 4.0f, (a.z + b.z + c.z + d.z) / 4.0f };
	return info;
}

inline CollisionInfo NextSimplex(Simplex& points, glm::vec3& direction) {
	switch (points.size()) {
	case 2: return Line(points, direction);
	case 3: return Triangle(points, direction);
	case 4: return Tetrahedron(points, direction);
	}

	// never should be here
	CollisionInfo info;
	info.colliding = false;
	return info;
}

inline CollisionInfo GJK(const Collider* colliderA, const Collider* colliderB)
{
	// Get initial support point in any direction
	glm::vec3 support = Support(colliderA, colliderB, glm::vec3{ 1, 0, 0 });

	// Simplex is an array of points, max count is 4
	Simplex points;
	points.push_front(support);

	// New direction is towards the origin
	glm::vec3 direction = -support;

	while (true) {
		support = Support(colliderA, colliderB, direction);

		if (glm::dot(support, direction) <= 0) {
			//return false; // no collision
			CollisionInfo info;
			info.colliding = false;
			return info;
		}

		points.push_front(support);

		CollisionInfo info = NextSimplex(points, direction);

		if (info.colliding) {

			return info;
		}
	}
}