#pragma once

#include "rtweekend.h"
#include "hittable.h"

class moving_sphere : public hittable {
	public:
		moving_sphere() {}
		moving_sphere(point3 cen0, point3 cen1, double _time0, double _time1, double r, shared_ptr<material> m) : 
			center0(cen0) , center1(cen1) , time0(_time0) , time1(_time1) , radius(r),mat_ptr(m)
		{};

		virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override;
		virtual bool bounding_box(double time0, double time1, aabb& output_box) const override;
		point3 center(double time) const;

	public:
		point3 center0 , center1;
		double time0, time1;
		double radius;
		shared_ptr<material> mat_ptr;
};

inline bool moving_sphere::hit(const ray& r, double t_min, double t_max, hit_record& rec) const
{
	vec3 oc = r.origin() - center(r.time());
	auto a = r.direction().length_squared();
	auto half_b = dot(oc,r.direction());
	auto c = oc.length_squared() - radius * radius;

	auto discriminant = half_b * half_b - a * c;

	if (discriminant < 0) {
		return false;
	}
	auto sqrtd = sqrt(discriminant);
	auto root = (-half_b - sqrtd) / a;
	if (root < t_min || root > t_max) {
		root = (-half_b + sqrtd) / a;
		if (root < t_min || root > t_max) {
			return false;
		}
	}

	rec.t = root;
	rec.p = r.at(rec.t);
	auto outward_normal = (rec.p - center(r.time())) / radius;
	rec.set_face_normal(r,outward_normal);
	rec.mat_ptr = mat_ptr;

	return true;
}
 
point3 moving_sphere::center(double time) const {
	return center0 + ((time - time0) / (time1 - time0)) * (center1 - center0);
}

//对于运动的球体，需要先计算t0时刻的包围盒，再计算t1时刻的包围盒，最后再用一个函数计算这俩包围盒的包围盒
bool moving_sphere::bounding_box(double time0, double time1, aabb& output_box) const {
	aabb box0(
		center(time0) - vec3(radius),
		center(time0) + vec3(radius)
	);
	aabb box1(
		center(time1) - vec3(radius),
		center(time1) + vec3(radius)
	);

	//将计算结果的包围盒返回给output_box
	output_box = surrounding_box(box0,box1);
	return true;
}