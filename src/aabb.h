#pragma once

#include "rtweekend.h"

class aabb {
public:
	aabb() {} 
	aabb(const vec3& a,const vec3& b) : _min(a),_max(b) {}

	vec3 getMin() { return _min; }
	vec3 getMax() { return _max; }

	bool hit(const ray& r,double tmin,double tmax) const {
		for (int a = 0; a < 3; a++) {
			auto invD = 1.0f / r.direction()[a];
			auto t0 = (_min[a] - r.origin()[a]) * invD;
			auto t1 = (_max[a] - r.origin()[a]) * invD;
			if (invD < 0.0f) {
				std::swap(t0,t1);
			}
			tmin = t0 > tmin ? t0 : tmin;	//ȡ������ֵ
			tmax = t1 < tmax ? t1 : tmax;	//ȡ�Ҷ���Сֵ
			if (tmax <= tmin) {		//ֻҪ������ֵ С�� �Ҷ���Сֵ  ���߾ͺ�aabb�ཻ
				return false;
			} 
		}
		return true;
	}

	//������˶���������������Χ�У�����������Χ�еİ�Χ��
	

public:
	vec3 _min;
	vec3 _max;
};

aabb surrounding_box(aabb box0, aabb box1) {
	point3 small(
		fmin(box0.getMin().x(), box1.getMin().x()),
		fmin(box0.getMin().y(), box1.getMin().y()),
		fmin(box0.getMin().z(), box1.getMin().z())
	);

	point3 big(
		fmax(box0.getMax().x(), box1.getMax().x()),
		fmax(box0.getMax().y(), box1.getMax().y()),
		fmax(box0.getMax().z(), box1.getMax().z())
	);

	return aabb(small, big);
}