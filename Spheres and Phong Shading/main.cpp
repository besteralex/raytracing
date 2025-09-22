/**
@file main.cpp
*/

#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>
#include <vector>
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"

#include "Image.h"
#include "Material.h"

using namespace std;

/**
 Class representing a single ray.
 */
class Ray{
public:
    glm::vec3 origin; ///< Origin of the ray
    glm::vec3 direction; ///< Direction of the ray
	/**
	 Constructor of the ray
	 @param origin Origin of the ray
	 @param direction Direction of the ray
	 */
    Ray(glm::vec3 origin, glm::vec3 direction) : origin(origin), direction(direction){
    }
};

class Object;

/**
 Stores the details of a ray intersection
 */
struct Hit{
    bool hit; ///< Boolean indicating whether there was or there was no intersection with an object
    glm::vec3 normal; ///< Normal vector of the intersected object at the intersection point
    glm::vec3 intersection; ///< Point of Intersection
    float distance; ///< Distance from the origin of the ray to the intersection point
    Object *object; ///< A pointer to the intersected object
};

/**
 General class for the object
 */
class Object{
	
public:
	glm::vec3 color; ///< Color of the object
	Material material; ///< Structure describing the material of the object
	/** A function computing an intersection, which returns the structure Hit */
    virtual Hit intersect(Ray ray) = 0;

	/** Function that returns the material struct of the object*/
	Material getMaterial(){
		return material;
	}
	/** Function that set the material
	 @param material A structure describing the material of the object
	*/
	void setMaterial(Material material){
		this->material = material;
	}
};

/**
 Implementation of the class Object for sphere shape.
 */
class Sphere : public Object{
private:
    float radius; ///< Radius of the sphere
    glm::vec3 center; ///< Center of the sphere

public:
	/**
	 The constructor of the sphere
	 @param radius Radius of the sphere
	 @param center Center of the sphere
	 @param color Color of the sphere
	 */
    Sphere(float radius, glm::vec3 center, glm::vec3 color) : radius(radius), center(center){
		this->color = color;
    }
	Sphere(float radius, glm::vec3 center, Material material) : radius(radius), center(center){
		this->material = material;
	}
	/** Implementation of the intersection function*/
	Hit intersect(Ray ray){
		
		Hit hit;
		hit.hit = false;

		// Project the sphere center onto the ray.

		glm::vec3 to_center = this->center - ray.origin;
		glm::vec3 sphere_center = this->center;
		glm::vec3 ray_direction = ray.direction;
		double center_projection = glm::dot(to_center, ray_direction);
		double center_distance = glm::length(to_center);
		double distance_to_ray = glm::sqrt(glm::pow(center_distance, 2) - glm::pow(center_projection, 2));
		double sphere_radius = (double) this->radius;
		
		if(distance_to_ray < sphere_radius) {
			double half_chord_length = glm::sqrt(glm::pow(sphere_radius, 2) - glm::pow(distance_to_ray, 2));

			// The two distances describe the far and near sphere intersections.
			double far_distance = center_projection + half_chord_length;
			double near_distance = center_projection - half_chord_length;

			// Ignore intersections behind the ray origin.
			if(near_distance < 0 && far_distance < 0) {
				hit.hit = false;
				return hit;
			} 	else if(near_distance > 0) {
				
				hit.distance = near_distance;
				hit.intersection = ray.origin + ray_direction * (float)near_distance;
				hit.hit = true;
				hit.normal = (hit.intersection - sphere_center);
				hit.object = this;
			} else if (far_distance > 0) {
				hit.distance = far_distance;
				hit.intersection = ray.origin + ray_direction * (float)far_distance;
				hit.hit = true;
				hit.normal = (hit.intersection - sphere_center);
				hit.object = this;
			}
			
		} else if (distance_to_ray == sphere_radius) {
			if(center_projection < 0) {
				return hit;
			}

			hit.distance = center_projection;
			hit.intersection = ray.origin + ray_direction * (float)center_projection;
			hit.hit = true;
			
			hit.normal = glm::normalize(hit.intersection - sphere_center);
			hit.object = this;
	}	
		return hit;
	}
};

/**
 Light class
 */
class Light{
public:
	glm::vec3 position; ///< Position of the light source
	glm::vec3 color; ///< Color/intensity of the light source
	Light(glm::vec3 position): position(position){
		color = glm::vec3(1.0);
	}
	Light(glm::vec3 position, glm::vec3 color): position(position), color(color){
	}
};

vector<Light *> lights; ///< A list of lights in the scene
glm::vec3 ambient_light(0.1,0.1,0.1);
vector<Object *> objects; ///< A list of all objects in the scene

/** Function for computing color of an object according to the Phong Model
 @param point A point belonging to the object for which the color is computed
 @param normal A normal vector at the point
 @param view_direction A normalized direction from the point to the viewer/camera
 @param material A material structure representing the material of the object
*/
glm::vec3 PhongModel(glm::vec3 point, glm::vec3 normal, glm::vec3 view_direction, Material material){

	glm::vec3 color(0.0);

	glm::vec3 ambient_color = material.ambient;
	glm::vec3 diffuse_color = material.diffuse;
	glm::vec3 specular_color = material.specular;
	float shininess = material.shininess;
	glm::vec3 surface_normal = normal;
	glm::vec3 view_vector = view_direction;

	// Add ambient light before the per-light contributions.
	glm::vec3 ambient_factor = ambient_light * ambient_color;

	color += ambient_factor;

	for(int light_index = 0; light_index < lights.size(); light_index++) {
		Light* light = lights[light_index];
		glm::vec3 light_color = light->color;
		// The light color stores its intensity in each color channel.

		// Calculate the diffuse contribution.
		glm::vec3 to_light = light->position - point;
		glm::vec3 light_direction = glm::normalize(to_light);

		// Clamp the diffuse contribution when the light is behind the surface.
		float cos_diffuse = glm::dot(light_direction,normal);
		if(cos_diffuse < 0) {
			cos_diffuse = 0;
		}
		glm::vec3 diffuse_factor = diffuse_color * cos_diffuse * light_color;

		// Use the halfway direction for the specular highlight.
		glm::vec3 half_vector = glm::normalize((glm::vec3(0.5, 0.5, 0.5)) * (light_direction + view_vector));
		float normal_dot_half = glm::dot(half_vector, surface_normal);
		float cos_specular = glm::pow(normal_dot_half, 4 * shininess);
		if(cos_specular < 0) {
			cos_specular = 0;
		}
		glm::vec3 specular_factor = specular_color * cos_specular * light_color;

		color += diffuse_factor + specular_factor;

	 }
	
	// The final color has to be clamped so the values do not go beyond 0 and 1.

	color = glm::clamp(color, glm::vec3(0.0), glm::vec3(1.0));
	return color;
}
