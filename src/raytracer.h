#pragma once

#include "rtweekend.h"
#include "hittable_list.h"
#include "sphere.h"
#include "camera.h"
#include "material.h"
#include "moving_sphere.h"
#include "bvh_node.h"
#include "ThreadPool.h"

color ray_color(const ray& r, const hittable& world, int depth) {
	hit_record rec;

	// If we've exceeded the ray bounce limit, no more light is gathered.
	if (depth <= 0)
		return color(0, 0, 0);

	if (world.hit(r, 0.001, infinity, rec)) {
		ray scattered;
		color attenuation;
		if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
			return attenuation * ray_color(scattered, world, depth - 1);
		return color(0, 0, 0);
	}
	vec3 unit_direction = unit_vector(r.direction());
	auto t = 0.5 * (unit_direction.y() + 1.0);
	return (1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0);
}

// screen
auto aspect_ratio = 16.0 / 9.0;
int image_width = 400;
int image_height = static_cast<int>(image_width / aspect_ratio);
int samples_per_pixel = 100;
const int max_depth = 50;

//picture id
int pic_id = 0;

// camera
point3 lookfrom(13, 2, 3);
point3 lookat(0, 0, 0);
vec3 vup(0, 1, 0);
float vfov = 20;
float dist_to_focus = 10.0f;
float aperture = 0.1f;
color ground(1.0,1.0,1.0);
// multi-threading
ThreadPool pool(std::thread::hardware_concurrency());	//�����̳߳أ������̳߳صĴ�С����ΪӲ���Ĳ�����
std::mutex tile_mutex;	//�����˻�����󣨶��߳��±�֤�ٽ�����ȫ��ͬ�����ƣ�
int finishedTileCount = 0;
int totalTileCount = 0;
double startTime = 0;
int tileSize = 16;	//ÿ��С����

class raytracer {
	hittable_list hworld;
	camera cam;
	uint8_t* pixels = nullptr;
	bvh_node bvh;

public:

	bvh_node setBVH() {
		bvh_node _bvh = bvh_node(hworld,0.0,1.0);
		return _bvh;
	}

	void write_color(color pixel_color, int i, int j)
	{
		auto r = pixel_color.x();
		auto g = pixel_color.y();
		auto b = pixel_color.z();

		// Divide the color by the number of samples and gamma-correct for gamma=2.0.
		auto scale = 1.0 / samples_per_pixel;
		r = sqrt(scale * r);
		g = sqrt(scale * g);
		b = sqrt(scale * b);

		// Write the translated [0,255] value of each color component.
		int index = i + j * image_width;
		pixels[index * 4] = static_cast<int>(256 * clamp(r, 0.0, 0.999));
		pixels[index * 4 + 1] = static_cast<int>(256 * clamp(g, 0.0, 0.999));
		pixels[index * 4 + 2] = static_cast<int>(256 * clamp(b, 0.0, 0.999));
		pixels[index * 4 + 3] = 255;
	}

	hittable_list two_sphere() {
		hittable_list objects;
		auto checker = make_shared<checker_texture>(color(0.2,0.3,0.1) , color(0.9));

		objects.add(make_shared<sphere>(point3(0.-10.0), 10,make_shared<lambertian>(checker)));
		objects.add(make_shared<sphere>(point3(0,10,0),10,make_shared<lambertian>(checker)));

		return objects;
	}

	hittable_list init_render()
	{
		hittable_list world;
		// World
		auto R = cos(pi / 4);

		auto checker = make_shared<checker_texture>(color(0.2,0.3,0.1) , color(0.9,0.9,0.9));
		world.add(make_shared<sphere>(point3(0, -1000, 0), 1000,make_shared<lambertian>(checker)));

		for (int a = -11; a < 11; a++) {
			for (int b = -11; b < 11; b++) {
				auto choose_mat = random_double();
				point3 center(a + 0.9 * random_double(), 0.2, b + 0.9 * random_double());

				if ((center - point3(4, 0.2, 0)).length() > 0.9) {
					shared_ptr<material> sphere_material;

					if (choose_mat < 0.8) {
						// diffuse
						auto albedo = color::random() * color::random();
						sphere_material = make_shared<lambertian>(albedo);
						auto center2 = center + vec3(0, random_double(0, .5), 0);
						world.add(make_shared<moving_sphere>(center , center2, 0.0, 1.0,0.2, sphere_material));
					}
					else if (choose_mat < 0.95) {
						// metal
						auto albedo = color::random(0.5, 1);
						auto fuzz = random_double(0, 0.5);
						sphere_material = make_shared<metal>(albedo, fuzz);
						world.add(make_shared<sphere>(center, 0.2, sphere_material));
					}
					else {
						// glass
						sphere_material = make_shared<dielectric>(1.5);
						world.add(make_shared<sphere>(center, 0.2, sphere_material));
					}
				}
			}
		}

		auto material1 = make_shared<dielectric>(1.5);
		world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

		auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
		world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

		auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
		world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

		// Camera
		cam.init(lookfrom, lookat, vup, vfov, aspect_ratio , aperture, dist_to_focus , 0.0,1.0);
		return world;
	}

	void render(uint8_t* _pixels)
	{
		pixels = _pixels;
		startTime = glfwGetTime();

		switch (pic_id) {
			case 1:
				hworld = init_render();
				lookfrom = point3(13,2,3);
				lookat = point3(0);
				vfov = 20.0;
				aperture = 0.1;
				cam.init(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus, 0.0, 1.0);
				break;
			case 2:
				hworld = two_sphere();
				lookfrom = point3(13, 2, 3);
				lookat = point3(0);
				vfov = 20.0;
				aperture = 0.0;
				cam.init(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, dist_to_focus, 0.0, 1.0);
				break;
		}

		// world and camera
		bvh = setBVH();

		int xTiles = (image_width + tileSize - 1) / tileSize;	//������� ���ΪС�飬+С��size-1��Ϊ�������һ�����ʣ�ಿ��
		int yTiles = (image_height + tileSize - 1) / tileSize;

		totalTileCount = xTiles * yTiles;	//�ܹ��ж��ٿ�
		finishedTileCount = 0;	//�Ѿ��������С������

		//renderTileÿ�ε��ã��ڲ���������Ⱦһ��С�顣���·���enqueue���ݺ���
		auto renderTile = [&](int xTile, int yTile) {	//lambda��������Ⱦһ��С�飬����������ص�ʹ��
			int xStart = xTile * tileSize;
			int yStart = yTile * tileSize;
			for (int j = yStart; j < yStart + tileSize; j++)	//��ʼ����һ��С��
			{
				for (int i = xStart; i < xStart + tileSize; i++)
				{
					// bounds check
					if (i >= image_width || j >= image_height)	//��ǰС�鳬��ͼƬ��Ͳ���Ⱦ����break����Ϊ�����ˣ��߻�û�����꣩
						continue;

					color pixel_color(0, 0, 0);
					for (int s = 0; s < samples_per_pixel; s++) {	//��һ�����ؽ��ж�β���
						auto u = (i + random_double()) / (image_width - 1);	//u��vֵ����0~1֮�䣬����һ���������Ϊ����һ�������ڽ����������
						auto v = (j + random_double()) / (image_height - 1);	//-1����Ϊ�����±��Ǵ�0��ʼ�ģ�����image�Ŀ��Ҫ-1��ͬ��
						ray r = cam.get_ray(u, v);	//����һ������
						pixel_color += ray_color(r, bvh, max_depth);	//��������ɫֵ��+��һ�������ƽ��
					}

					write_color(pixel_color, i, j);
				}
			}
			{
				std::lock_guard<std::mutex> lock(tile_mutex);
				finishedTileCount++;
				if (finishedTileCount == totalTileCount)
				{
					std::cout << "render async finished, spent " << glfwGetTime() - startTime << "s." << std::endl;
				}
			}
		};

		for (int i = 0; i < xTiles; i++)
		{
			for (int j = 0; j < yTiles; j++)
			{
				pool.enqueue(renderTile, i, j);	//enqueue�ĺ���������Ϊ��һ������ָ��Ĳ���
			}
		}
	}

	void render_sync(uint8_t* _pixels)	//ͬ������
	{
		//pixels = _pixels;
		//startTime = glfwGetTime();

		//// world and camera
		//init_render();

		//for (int j = 0; j < image_height; j++)
		//{
		//	for (int i = 0; i < image_width; i++) // go horizontal line first
		//	{
		//		color pixel_color(0, 0, 0);
		//		for (int s = 0; s < samples_per_pixel; ++s) {
		//			auto u = (i + random_double()) / (image_width - 1);
		//			auto v = (j + random_double()) / (image_height - 1);
		//			ray r = cam.get_ray(u, v);
		//			pixel_color += ray_color(r, world, max_depth);
		//		}

		//		write_color(pixel_color, i, j);
		//	}
		//}
		std::cout << "render sync finished, spent " << glfwGetTime() - startTime << "s." << std::endl;
	}
};