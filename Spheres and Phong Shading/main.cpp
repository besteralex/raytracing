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

/**
 Computes a color along the ray
 @param ray Ray that should be traced through the scene
 @return Color at the intersection point
 */
glm::vec3 trace_ray(Ray ray){

	Hit closest_hit;

	closest_hit.hit = false;
	closest_hit.distance = INFINITY;

	// Keep the closest intersection found along the ray.
	for(int object_index = 0; object_index<objects.size(); object_index++){
		Hit hit = objects[object_index]->intersect(ray);
		if(hit.hit && hit.distance < closest_hit.distance)
			closest_hit = hit;
	}

	glm::vec3 color(0.0);

	if(closest_hit.hit) {

		color = PhongModel(closest_hit.intersection, closest_hit.normal, glm::normalize(-ray.direction), closest_hit.object->getMaterial());
	}else{
		color = glm::vec3(0.0, 0.0, 0.0);
	}
	return color;
}
/**
 Function defining the scene
 */
void sceneDefinition () {

    Material red_specular;

	// Material settings for the red sphere.
    red_specular.ambient = glm::vec3(0.01f, 0.03f, 0.03f);
    red_specular.diffuse = glm::vec3(1.0f, 0.3f, 0.3f);
    red_specular.specular = glm::vec3(0.5f, 0.5f, 0.5f);
    red_specular.shininess = 10.0; // Controls the specular highlight.

    Material green_specular;
    green_specular.ambient = glm::vec3(0.07f, 0.09f, 0.07f);
    green_specular.diffuse = glm::vec3(0.7f, 0.9f, 0.7f);
    green_specular.specular = glm::vec3(0.0f, 0.0f, 0.0f);
    green_specular.shininess = 0.0;

	Material blue_specular;
    blue_specular.ambient = glm::vec3(0.07f, 0.07f, 0.1f);
    blue_specular.diffuse = glm::vec3(0.7f, 0.7f, 1.0f);
    blue_specular.specular = glm::vec3(0.6f, 0.6f, 0.6f);
    blue_specular.shininess = 100.0;

	objects.push_back(new Sphere(1.0, glm::vec3(1.0, -2.0, 8.0), blue_specular));
    objects.push_back(new Sphere(0.5, glm::vec3(-1.0, -2.5, 6.0), red_specular));
    objects.push_back(new Sphere(1.0, glm::vec3(2.0, -2.0, 6.0), green_specular));

	lights.push_back(new Light(glm::vec3(0.0, 26.0, 5.0), glm::vec3(0.4)));
	lights.push_back(new Light(glm::vec3(0.0, 1.0, 12.0), glm::vec3(0.4)));
	lights.push_back(new Light(glm::vec3(0.0, 5.0, 1.0), glm::vec3(0.4)));

}

int main(int argc, const char * argv[]) {

    clock_t render_ticks = clock(); // Tracks rendering time.

    int width = 1024; //width of the image
    int height = 768; // height of the image
    float field_of_view = 90; // Field of view in degrees.
	sceneDefinition(); // Set up scene objects and lights.

	Image image(width,height); // Create an image where we will store the result

	glm::vec3 camera_origin = glm::vec3(0, 0, 0);
	float pixel_size = 2.0f * glm::tan(glm::radians(field_of_view * 0.5f)) / width;
	double image_left = (width * pixel_size) / 2 * (-1);
	double image_top = (height * pixel_size) / 2;
    for(int pixel_x = 0; pixel_x < width ; pixel_x++)
        for(int pixel_y = 0; pixel_y < height ; pixel_y++){

			// Aim each ray through the center of its pixel.
			double ray_x = image_left + pixel_x * pixel_size + 0.5 * pixel_size;
			double ray_y = image_top - pixel_y * pixel_size - 0.5 * pixel_size;
			double ray_z = 1;
			glm::vec3 unnormalized_direction = glm::vec3(ray_x, ray_y, ray_z);
			glm::vec3 direction = glm::normalize(unnormalized_direction);
			Ray ray(camera_origin, direction);
			image.setPixel(pixel_x, pixel_y, trace_ray(ray));
        }

    render_ticks = clock() - render_ticks;
    cout<<"It took " << ((float)render_ticks)/CLOCKS_PER_SEC<< " seconds to render the image."<< endl;
    cout<<"I could render at "<< (float)CLOCKS_PER_SEC/((float)render_ticks) << " frames per second."<<endl;

	// Save the image to the requested path, or use result.ppm.
	if (argc == 2){
		image.writeImage(argv[1]);
	}else{
		image.writeImage("./result.ppm");
	}	
    return 0;
}
