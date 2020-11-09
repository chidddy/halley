/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/


#include "halley/maths/polygon.h"
#include <limits>

#include "halley/file_formats/config_file.h"
#include "halley/maths/ray.h"
#include "halley/maths/circle.h"
#include "halley/maths/line.h"
using namespace Halley;


///////////////
// Constructor
Polygon::Polygon()
{
}

Polygon::Polygon(VertexList vertices)
	: vertices(std::move(vertices))
{
	realize();
}


//////////////////////////////////////////////
// Realize that the polygon has changed shape
void Polygon::realize()
{
	aabb = Rect4f::getSpanningRect(vertices);
	circle = Circle::getSpanningCircle(vertices);
}



///////////////////////////////////////////////////////
// Checks if a particular point is inside the polygon
// Only works for convex polygons
bool Polygon::isPointInsideConvex(Vector2f point) const
{
	// Fast fail
	if (!circle.contains(point)) {
		return false;
	}

	// Do cross product with all the segments
	const size_t len = vertices.size();
	for (size_t i = 0; i < len; i++) {
		const auto a = point - vertices[i];
		const auto b = vertices[(i+1) % len] - vertices[i];
		if (a.cross(b) > 0) {
			return false;
		}
	}

	// Nothing failed, so it's inside
	return true;
}

bool Polygon::isPointInside(Vector2f point) const
{
	// Fast fail
	if (!circle.contains(point)) {
		return false;
	}
	if (!aabb.contains(point)) {
		return false;
	}

	size_t nLeft = 0;
	size_t nRight = 0;
	const size_t len = vertices.size();

	// For each segment that overlaps this point vertically, classify it as "left" or "right"
	for (size_t i = 0; i < len; i++) {
		const auto a = vertices[i];
		const auto b = vertices[(i+1) % len];
		auto r = Range<float>(a.y, b.y);
		if (r.contains(point.y)) {
			if (a.x < point.x && b.x < point.x) {
				nLeft++;
			} else if (a.x > point.x && b.x > point.x) {
				nRight++;
			} else {
				const float t = (point.y - a.y) / (b.y - a.y);
				const float refX = lerp(a.x, b.x, t);
				if (refX < point.x) {
					nLeft++;
				} else {
					nRight++;
				}
			}
		}
	}

	return (nLeft % 2) == 1 && (nRight % 2) == 1;
}


Vector2f average(Vector<Vector2f>& v)
{
	Vector2f result;
	size_t len = v.size();
	for (size_t i=0; i<len; i++) {
		result += v[i];
	}
	result /= float(len);
	return result;
}


bool Polygon::overlaps(const Polygon &param,Vector2f *translation,Vector2f *collisionPoint) const
{
	// Using the separating axis theorem here
	// Check if they are within overlap range
	const float maxDist = circle.getRadius() + param.circle.getRadius();
	if ((circle.getCentre() - param.circle.getCentre()).squaredLength() >= maxDist * maxDist) {
		return false;
	}

	// AABB test
	//if (!aabb.overlaps(param.aabb, param.getOrigin()-getOrigin())) return false;

	// Prepare
	float minDist = -999999.0f;
	Vector2f bestAxis;
	bool hasBestAxis = false;
	float bmin1=0, bmax1=0, bmin2=0, bmax2=0;

	// For each edge
	size_t len1 = vertices.size();
	size_t len2 = param.vertices.size();
	for (size_t i=0; i<len1+len2; i++) {
		// Find the orthonormal axis
		Vector2f axis;
		if (i < len1) axis = (vertices[(i+1)%len1] - vertices[i]).orthoLeft().unit();
		else axis = (param.vertices[(i-len1+1)%len2] - param.vertices[i-len1]).orthoLeft().unit();

		// Project both polygons there
		float min1, max1, min2, max2;
		project(axis,min1,max1);
		param.project(axis,min2,max2);

		// Find the distance between the projections
		float dist = min1<min2 ? min2 - max1 : min1 - max2;
		if (dist >= 0) {
			// This axis separates them
			return false;
		} else {
			if (translation && dist > minDist) {
				bestAxis = axis;
				hasBestAxis = true;
				minDist = dist;
				bmin1 = min1;
				bmin2 = min2;
				bmax1 = max1;
				bmax2 = max2;
			}
		}
	}

	// Gather additional data based on best axis
	if (hasBestAxis) {
		// Find all vertices possibly involved in the collision
		float dist;
		int sign;
		Vector<Vector2f> v1,v2;
		if (bmin1 < bmin2) {
			dist = bmin2 - bmax1;
			sign = 1;
			if (collisionPoint) {
				unproject(bestAxis,bmax1,v1);
				param.unproject(bestAxis,bmin2,v2);
			}
		}
		else {
			dist = bmin1 - bmax2;
			sign = -1;
			if (collisionPoint) {
				unproject(bestAxis,bmin1,v1);
				param.unproject(bestAxis,bmax2,v2);
			}
		}

		// Find the collision point
		if (collisionPoint) {
			Vector2f colPoint = (circle.getCentre() + param.circle.getCentre()) / 2.0f;
			if (v1.size() == 1) {
				colPoint = v1[0];
			} else if (v2.size() == 1) {
				colPoint = v2[0];
			} else if (!v1.empty()) {
				colPoint = average(v1); //v1[0];
			} else if (!v2.empty()) {
				colPoint = average(v2); //v2[0];
			}
			*collisionPoint = colPoint;
		}

		// Find the translation vector
		*translation = bestAxis*(dist*sign);
	}

	// Done
	return true;
}

Vector2f Polygon::getClosestPoint(Vector2f rawPoint, float anisotropy) const
{
	Expects(!vertices.empty());

	const auto scale = Vector2f(1.0f, 1.0f / anisotropy);
	const auto point = rawPoint * scale;
	
	Vector2f bestPoint = vertices[0];
	float closestDistance2 = std::numeric_limits<float>::infinity();
	
	const size_t n = vertices.size();
	for (size_t i = 0; i < n; ++i) {
		const Vector2f p = LineSegment(vertices[i] * scale, vertices[(i + 1) % n] * scale).getClosestPoint(point);

		const float dist2 = (point - p).squaredLength();
		if (dist2 < closestDistance2) {
			closestDistance2 = dist2;
			bestPoint = p;
		}
	}

	return bestPoint * Vector2f(1.0f, anisotropy);
}


////////////////////////////////
// Project polygon into an axis
void Polygon::project(const Vector2f &_axis,float &_min,float &_max) const
{
	Vector2f axis = _axis;
	float dot = axis.dot(vertices[0]);
	float min = dot;
	float max = dot;
	size_t len = vertices.size();
	for (size_t i=1;i<len;i++) {
		dot = axis.dot(vertices[i]);
		if (dot < min) min = dot;
		else if (dot > max) max = dot;
	}
	_min = min;
	_max = max;
}


/////////////
// Unproject
// Finds all vertices whose projection on a given axis is the value given
void Polygon::unproject(const Vector2f &axis,const float point,Vector<Vector2f> &ver) const
{
	size_t len = vertices.size();
	float dot;
	for (size_t i=0;i<len;i++) {
		dot = axis.dot(vertices[i]);
		if (dot == point) ver.push_back(vertices[i]);
	}
}


//////////
// Rotate
// Return this polygon rotated by angle
void Polygon::rotate(Angle<float> angle)
{
	size_t len = vertices.size();
	for (size_t i=0;i<len;i++) {
		vertices[i] = vertices[i].rotate(angle);
	}
	realize();
}


void Halley::Polygon::rotateAndScale(Angle<float> angle, Vector2f scale)
{
	size_t len = vertices.size();
	for (size_t i=0;i<len;i++) {
		vertices[i] = (vertices[i] * scale).rotate(angle);
	}
	realize();
}


Halley::Polygon Polygon::makePolygon(Vector2f origin, float w, float h)
{
	const float x = origin.x;
	const float y = origin.y;
	VertexList list;
	list.push_back(Vertex(x, y));
	list.push_back(Vertex(x+w, y));
	list.push_back(Vertex(x+w, y+h));
	list.push_back(Vertex(x, y+h));
	return Halley::Polygon(list);
}


bool Polygon::isClockwise() const
{
	if (vertices.size() < 3) return true;

	return ((vertices[1]-vertices[0]).cross(vertices[2]-vertices[1]) > 0);
}

void Polygon::setVertices(const VertexList& _vertices)
{
	vertices = _vertices;
	realize();
}

void Polygon::translate(Vector2f offset)
{
	for (auto& v: vertices) {
		v += offset;
	}
	realize();
}


Polygon::CollisionResult Polygon::getCollisionWithSweepingCircle(Vector2f p0, float radius, Vector2f moveDir, float moveLen) const
{
	CollisionResult result;

	// This is used to grow AABBs to check if p0 is inside
	// If this coarse test fails, the sweep shouldn't overlap the polygon
	const float border = radius + (moveLen * std::max(std::abs(moveDir.x), std::abs(moveDir.y)));
	if (!getAABB().grow(border).contains(p0)) {
		result.fastFail = true;
		return result;
	}

	const auto submit = [&] (std::optional<std::pair<float, Vector2f>> c)
	{
		if (c && c->first < moveLen) {
			if (!result.collided || c->first < result.distance) {
				result.collided = true;
				result.distance = c->first;
				result.normal = c->second;
			}
		}
	};
	
	const auto ray = Ray(p0, moveDir);

	for (size_t i = 0; i < vertices.size(); ++i) {
		// For each line segment in the polygon...
		const Vector2f a = vertices[i];
		const Vector2f b = vertices[(i + 1) % vertices.size()];

		// We expand the line segment into a rounded capsule.
		// It's now two circles (one centred at each vertex) and two line segments (connecting the circles)
		// Checking collision of this capsule against the centre of the circle is isomorphic to the original problem, but easier

		// Check circles
		// Only check one vertex, "b" will be checked by another iteration
		submit(ray.castCircle(a, radius));

		// Check segments
		// One of the two line segments (facing away) is not needed, so we only test two circles and one segment
		Vector2f offset = (a - b).normalized().orthoLeft() * radius;
		if (offset.dot(moveDir) > 0) {
			offset = -offset;
		}
		submit(ray.castLineSegment(a + offset, b + offset));
	}
	
	return result;
}

Polygon::CollisionResult Polygon::getCollisionWithSweepingEllipse(Vector2f p0, Vector2f radius, Vector2f moveDir, float moveLen) const
{
	// This is the same algorithm as above, but we scale everything so the ellipse becomes a circle
	CollisionResult result;
	
	// This is used to grow AABBs to check if p0 is inside
	// If this coarse test fails, the sweep shouldn't overlap the polygon
	const float border = std::max(radius.x, radius.y) + (moveLen * std::max(std::abs(moveDir.x), std::abs(moveDir.y)));
	if (!getAABB().grow(border).contains(p0)) {
		result.fastFail = true;
		return result;
	}

	const auto localRadius = radius.x;
	const auto scale = radius.x / radius.y;
	const auto transformation = Vector2f(1.0f, scale);

	const auto localMove = moveDir * transformation * moveLen;
	const auto localMoveLen = localMove.length();
	const auto localMoveDir = localMove.normalized();
	const auto localP0 = p0 * transformation;
	const auto ray = Ray(localP0, localMoveDir);

	float bestLen = localMoveLen;
	const auto submit = [&] (std::optional<std::pair<float, Vector2f>> c)
	{
		if (c) {
			const float lenToCol = c->first;
			if (lenToCol < bestLen) {
				result.collided = true;
				result.distance = c->first;
				result.normal = c->second;
				bestLen = lenToCol;
			}
		}
	};

	for (size_t i = 0; i < vertices.size(); ++i) {
		// For each line segment in the polygon...
		const Vector2f a = vertices[i] * transformation;
		const Vector2f b = vertices[(i + 1) % vertices.size()] * transformation;

		// We expand the line segment into a rounded capsule.
		// It's now two circles (one centred at each vertex) and two line segments (connecting the circles)
		// Checking collision of this capsule against the centre of the circle is isomorphic to the original problem, but easier

		// Check circle
		// Only check one vertex, "b" will be checked by another iteration
		submit(ray.castCircle(a, localRadius));

		// Check segments
		// One of the two line segments (facing away) is not needed, so we only test two circles and one segment
		Vector2f offset = (a - b).normalized().orthoLeft() * localRadius;
		if (offset.dot(localMoveDir) > 0) {
			offset = -offset;
		}
		submit(ray.castLineSegment(a + offset, b + offset));
	}

	if (result.collided) {
		// Transform the results back to global space
		result.distance *= moveLen / localMoveLen;

		// This is a multiply instead of the divide you might expect
		// The correct operation here is (norm.orthoLeft() / transform).orthoRight().normalized()
		// But this is equivalent and faster
		result.normal = (result.normal * transformation).normalized();
	}
	return result;
}

bool Polygon::operator==(const Polygon& other) const
{
	return vertices == other.vertices;
}

bool Polygon::operator!=(const Polygon& other) const
{
	return vertices != other.vertices;
}

ConfigNode ConfigNodeSerializer<Polygon>::serialize(const Polygon& polygon, const ConfigNodeSerializationContext&)
{
	return ConfigNode(polygon.getVertices());
}

Polygon ConfigNodeSerializer<Polygon>::deserialize(const ConfigNodeSerializationContext&, const ConfigNode& node) 
{
	VertexList list;
	if (node.getType() == ConfigNodeType::Sequence) {
		list.reserve(node.asSequence().size());
		for (auto& n: node.asSequence()) {
			list.push_back(n.asVector2f());
		}
	}
	return Polygon(std::move(list));
}
