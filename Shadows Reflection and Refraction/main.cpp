/**
@file main.cpp
*/

#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>
#include <vector>
#include "glm/common.hpp"
#include "glm/exponential.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/geometric.hpp"
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
	
protected:
	glm::mat4 transformationMatrix; ///< Matrix representing the transformation from the local to the global coordinate system
	glm::mat4 inverseTransformationMatrix; ///< Matrix representing the transformation from the global to the local coordinate system
	glm::mat4 normalMatrix; ///< Matrix for transforming normal vectors from the local to the global coordinate system
	
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
	/** Functions for setting up all the transformation matrices
	@param matrix The matrix representing the transformation of the object in the global coordinates */
	void setTransformation(glm::mat4 matrix){
			
		transformationMatrix = matrix;

		inverseTransformationMatrix = glm::inverse(matrix);
		normalMatrix = glm::transpose(inverseTransformationMatrix);
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

        glm::vec3 to_center = center - ray.origin;

        // Project the sphere center onto the ray to find the intersection distances.
        float center_distance_squared = glm::dot(to_center,to_center);
        float center_projection = glm::dot(to_center, ray.direction);

        Hit hit;

        float distance_to_ray = 0;
		if (center_distance_squared > center_projection*center_projection){
			distance_to_ray =  sqrt(center_distance_squared - center_projection*center_projection);
		}
        if(distance_to_ray<=radius){
            hit.hit = true;
            float near_distance = center_projection - sqrt(radius*radius - distance_to_ray*distance_to_ray);
            float far_distance = center_projection + sqrt(radius*radius - distance_to_ray*distance_to_ray);

            float ray_distance = near_distance;
            if(ray_distance<0) ray_distance = far_distance;
            if(ray_distance<0){
                hit.hit = false;
                return hit;
            }

			hit.intersection = ray.origin + ray_distance * ray.direction;
			hit.normal = glm::normalize(hit.intersection - center);
			hit.distance = glm::distance(ray.origin, hit.intersection);
			hit.object = this;
        } else{
            hit.hit = false;
		}
		return hit;
    }
};

class Plane : public Object{

private:
	glm::vec3 normal;
	glm::vec3 point;

public:
	Plane(glm::vec3 point, glm::vec3 normal) : point(point), normal(normal){
	}
	Plane(glm::vec3 point, glm::vec3 normal, Material material) : point(point), normal(normal){
		this->material = material;
	}
	Hit intersect(Ray ray){
		
		Hit hit;
		hit.hit = false;
		
        // This plane accepts rays arriving from its front side.
        float direction_dot_normal = glm::dot(ray.direction, normal);
        if(direction_dot_normal < 0){
            
            float offset_dot_normal = glm::dot (point-ray.origin, normal);
            float ray_distance = offset_dot_normal/direction_dot_normal;
            
            if(ray_distance > 0){
                hit.hit = true;
                hit.normal = normal;
                hit.distance = ray_distance;
                hit.object = this;
                hit.intersection = ray_distance * ray.direction + ray.origin;
            }
        }
		
		return hit;
	}
};

class Cone : public Object{
private:
	Plane *plane;
	
public:
	Cone(Material material){
		this->material = material;
		plane = new Plane(glm::vec3(0,1,0), glm::vec3(0.0,1,0));
	}
	Hit intersect(Ray ray){
		
		Hit hit;
		hit.hit = false;
		
		// Solve the intersection in the cone's local coordinate system.
		glm::vec3 local_direction = inverseTransformationMatrix * glm::vec4(ray.direction, 0.0); //implicit cast to vec3
		glm::vec3 local_origin = inverseTransformationMatrix * glm::vec4(ray.origin, 1.0); //implicit cast to vec3
		local_direction = glm::normalize(local_direction);
		
		
		float a = local_direction.x*local_direction.x + local_direction.z*local_direction.z - local_direction.y*local_direction.y;
		float b = 2 * (local_direction.x * local_origin.x + local_direction.z * local_origin.z - local_direction.y * local_origin.y);
		float c = local_origin.x * local_origin.x + local_origin.z * local_origin.z - local_origin.y * local_origin.y;
		
		float discriminant = b*b - 4 * a * c;
		
		if(discriminant < 0){
			return hit;
		}
		
		float first_root = (-b-sqrt(discriminant)) / (2*a);
		float second_root = (-b+sqrt(discriminant)) / (2*a);
		
		float ray_distance = first_root;
		hit.intersection = local_origin + ray_distance*local_direction;
		if(ray_distance<0 || hit.intersection.y>1 || hit.intersection.y<0){
			ray_distance = second_root;
			hit.intersection = local_origin + ray_distance*local_direction;
			if(ray_distance<0 || hit.intersection.y>1 || hit.intersection.y<0){
				return hit;
			}
		};
	
		hit.normal = glm::vec3(hit.intersection.x, -hit.intersection.y, hit.intersection.z);
		hit.normal = glm::normalize(hit.normal);
	
		
		// Check whether the cap is closer than the side intersection.
		Ray local_ray(local_origin,local_direction);
		Hit cap_hit = plane->intersect(local_ray);
		if(cap_hit.hit && cap_hit.distance < ray_distance && length(cap_hit.intersection - glm::vec3(0,1,0)) <= 1.0 ){
			hit.intersection = cap_hit.intersection;
			hit.normal = cap_hit.normal;
		}
		
		hit.hit = true;
		hit.object = this;
		// Convert the hit point and normal back to world coordinates.
		hit.intersection = transformationMatrix * glm::vec4(hit.intersection, 1.0); //implicit cast to vec3
		hit.normal = (normalMatrix * glm::vec4(hit.normal, 0.0)); //implicit cast to vec3
		hit.normal = glm::normalize(hit.normal);
		hit.distance = glm::length(hit.intersection - ray.origin);
		
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
// Ambient light used by the shading calculation.
glm::vec3 ambient_light(0.001,0.001,0.001);
vector<Object *> objects; ///< A list of all objects in the scene


// Return 1 when the light is visible from the surface point, or 0 when blocked.


int calc_light_s(Light* light, glm::vec3 point) {
	// Cast a ray from the surface toward the light.
	glm::vec3 light_direction = glm::normalize(light->position - point);
	Ray ray_from_point_to_light = Ray(point, light_direction);

	for(int object_index = 0; object_index<objects.size(); object_index++){
		Hit hit = objects[object_index]->intersect(ray_from_point_to_light);
		if(hit.hit && hit.distance < glm::distance(point, light->position) && hit.distance > 0.0001) {
			return 0;
		}
	}

	return 1;
}

glm::vec3 trace_ray(Ray ray, int reflection_depth, int refraction_depth);

// Combine direct lighting with reflected and refracted rays.
glm::vec3 PhongModel(glm::vec3 point, glm::vec3 normal, glm::vec3 view_direction, Material material, int reflection_depth, int refraction_depth){

	glm::vec3 color(0.0);
	glm::vec3 reflected_color(0.0);
	glm::vec3 refracted_color(0.0);
	for (int light_index = 0; light_index < lights.size(); light_index++){

		glm::vec3 light_direction = glm::normalize(lights[light_index]->position - point);
		glm::vec3 reflected_direction = glm::reflect(-light_direction, normal);

		float normal_dot_light = glm::clamp(glm::dot(normal, light_direction), 0.0f, 1.0f);
		float view_dot_reflection = glm::clamp(glm::dot(view_direction, reflected_direction), 0.0f, 1.0f);

		glm::vec3 diffuse_color = material.diffuse;
		glm::vec3 diffuse = diffuse_color * glm::vec3(normal_dot_light);
		glm::vec3 specular = material.specular * glm::vec3(pow(view_dot_reflection, material.shininess));
		
        float light_distance = glm::distance(point,lights[light_index]->position);
        light_distance = max(light_distance, 0.1f);
		// Only add direct light when the shadow ray is clear.
        color +=  glm::vec3(calc_light_s(lights[light_index], point)) * (lights[light_index]->color * (diffuse + specular)) / light_distance/light_distance;
	}

	glm::vec3 incident_direction = -view_direction;
	incident_direction = glm::normalize(incident_direction);
	glm::vec3 reflection_direction = glm::reflect(glm::normalize(incident_direction), normal);
	// Offset the reflected ray to avoid hitting the same surface again.
	Ray reflection_ray = Ray(point + reflection_direction * 1e-4f, reflection_direction);

	if(material.does_reflect && reflection_depth < 10) {
		reflected_color = trace_ray(reflection_ray, reflection_depth + 1, refraction_depth);
	} else {
		reflected_color = glm::vec3(0.0);
	}
	
	

	color += ambient_light * material.ambient;
	color = glm::clamp(color, glm::vec3(0.0), glm::vec3(1.0));
	reflected_color = glm::clamp(reflected_color, glm::vec3(0.0), glm::vec3(1.0));

	glm::vec3 normal_component = normal * glm::dot(normal, incident_direction);
	glm::vec3 tangent_component = incident_direction - normal_component;
	
	float incident_ior = 1.0;
	float transmitted_ior = material.refractive_index;
	float cos_incident_angle = glm::dot(incident_direction, normal);
	// Swap the refractive indices when the ray leaves the material.
	if(cos_incident_angle > 0) {
		incident_ior = material.refractive_index;
		transmitted_ior = 1.0;
	} else {
		incident_ior = 1.0;
		transmitted_ior = material.refractive_index;
	}

	cos_incident_angle = glm::abs(cos_incident_angle);
	float incident_angle = acos(glm::clamp(cos_incident_angle, 0.0f, 1.0f));
	float sin_incident_angle = sin(incident_angle);
	
	float sin_transmitted_angle = incident_ior / transmitted_ior * sin_incident_angle;
	float transmitted_angle = asin(sin_transmitted_angle);
	float cos_transmitted_angle = cos(transmitted_angle);

	float fresnel_term_1 = (incident_ior * cos_incident_angle - transmitted_ior * cos_transmitted_angle) / (incident_ior * cos_incident_angle + transmitted_ior * cos_transmitted_angle);
	float fresnel_term_2 = (incident_ior * cos_transmitted_angle - transmitted_ior * cos_incident_angle) / (incident_ior * cos_transmitted_angle + transmitted_ior * cos_incident_angle);

	// Average the two Fresnel terms to determine the reflection weight.
	float reflectance = 0.5 * (glm::pow(fresnel_term_1, 2) + glm::pow(fresnel_term_2, 2));

	// Use full reflection when the refraction condition is not met.
	if(glm::abs(incident_ior / transmitted_ior * sin(incident_angle)) >= 1) {
		reflectance = 1.0f;
	} 

	float ior_ratio = incident_ior / transmitted_ior;
	float normal_scale = glm::sqrt(1 + (1 - glm::pow(ior_ratio, 2)) * ((glm::pow(glm::length(tangent_component), 2) / glm::pow(glm::length(normal_component), 2))));
	glm::vec3 refraction_direction = glm::vec3(normal_scale) * normal_component + ior_ratio * tangent_component;
	Ray refraction_ray = Ray(point + refraction_direction * 1e-4f, glm::normalize(refraction_direction));

	// Track refraction depth separately from reflection depth.
	if(material.does_refract && refraction_depth < 10) {
		refracted_color = trace_ray(refraction_ray, reflection_depth, refraction_depth + 1);
	} else {
		refracted_color = glm::vec3(0.0f);
	}


	if(material.does_reflect && material.does_refract) {
		return reflected_color * (reflectance) + refracted_color * ((1 - reflectance));
	}

	if(material.does_reflect) {
		return reflected_color;
	}
	if(material.does_refract) {
		return refracted_color;
	}
	return color;
}

/**
 Computes a color along the ray
 @param ray Ray that should be traced through the scene
 @return Color at the intersection point
 */


glm::vec3 trace_ray(Ray ray, int reflection_depth, int refraction_depth){

	Hit closest_hit;

	closest_hit.hit = false;
	closest_hit.distance = INFINITY;

	for(int object_index = 0; object_index<objects.size(); object_index++){
		Hit hit = objects[object_index]->intersect(ray);
		if(hit.hit == true && hit.distance < closest_hit.distance)
			closest_hit = hit;
	}

	glm::vec3 color(0.0);
	if(closest_hit.hit){
		color = PhongModel(closest_hit.intersection, closest_hit.normal, glm::normalize(-ray.direction), closest_hit.object->getMaterial(), reflection_depth, refraction_depth);
	}else{
		color = glm::vec3(0.0, 0.0, 0.0);
	}
	return color;
}
/**
 Function defining the scene
 */
void sceneDefinition (){
	
	Material green_diffuse;
	green_diffuse.ambient = glm::vec3(0.7f, 0.9f, 0.7f);
	green_diffuse.diffuse = glm::vec3(0.7f, 0.9f, 0.7f);

	Material red_specular;
	red_specular.ambient = glm::vec3(1.0f, 0.3f, 0.3f);
	red_specular.diffuse = glm::vec3(1.0f, 0.3f, 0.3f);
	red_specular.specular = glm::vec3(0.5);
	red_specular.shininess = 10.0;

	Material blue_specular;
	blue_specular.ambient = glm::vec3(0.7f, 0.7f, 1.0f);
	blue_specular.diffuse = glm::vec3(0.7f, 0.7f, 1.0f);
	blue_specular.specular = glm::vec3(0.6);
	blue_specular.shininess = 100.0;

	Material green_refractive;
	green_refractive.ambient = glm::vec3(0.03f, 0.1f, 0.03f);
	green_refractive.diffuse = glm::vec3(0.3f, 1.0f, 0.3f);
	green_refractive.does_refract = true;
	green_refractive.does_reflect = true;
	green_refractive.refractive_index = 2.0f;
	
	
	green_diffuse.ambient = glm::vec3(0.03f, 0.1f, 0.03f);
	green_diffuse.diffuse = glm::vec3(0.3f, 1.0f, 0.3f);

	red_specular.diffuse = glm::vec3(1.0f, 0.2f, 0.2f);
	red_specular.ambient = glm::vec3(0.01f, 0.02f, 0.02f);
	red_specular.specular = glm::vec3(0.5);
	red_specular.shininess = 10.0;

	blue_specular.ambient = glm::vec3(0.02f, 0.02f, 0.1f);
	blue_specular.diffuse = glm::vec3(0.2f, 0.2f, 1.0f);
	blue_specular.specular = glm::vec3(0.6);
	blue_specular.shininess = 100.0;

	blue_specular.does_reflect = true;

	objects.push_back(new Sphere(1.0, glm::vec3(1,-2,8), blue_specular));
	objects.push_back(new Sphere(0.5, glm::vec3(-1,-2.5,6), red_specular));
	objects.push_back(new Sphere(2.0, glm::vec3(-3,-1,8), green_refractive));

	
	lights.push_back(new Light(glm::vec3(0, 26, 5), glm::vec3(1.0, 1.0, 1.0)));
	lights.push_back(new Light(glm::vec3(0, 1, 12), glm::vec3(0.1)));
	lights.push_back(new Light(glm::vec3(0, 5, 1), glm::vec3(0.4)));
	
    Material red_diffuse;
    red_diffuse.ambient = glm::vec3(0.09f, 0.06f, 0.06f);
    red_diffuse.diffuse = glm::vec3(0.9f, 0.6f, 0.6f);
        
    Material blue_diffuse;

    blue_diffuse.ambient = glm::vec3(0.06f, 0.06f, 0.09f);
    blue_diffuse.diffuse = glm::vec3(0.6f, 0.6f, 0.9f);
    objects.push_back(new Plane(glm::vec3(0,-3,0), glm::vec3(0.0,1,0)));
    objects.push_back(new Plane(glm::vec3(0,1,30), glm::vec3(0.0,0.0,-1.0), green_diffuse));
    objects.push_back(new Plane(glm::vec3(-15,1,0), glm::vec3(1.0,0.0,0.0), red_diffuse));
    objects.push_back(new Plane(glm::vec3(15,1,0), glm::vec3(-1.0,0.0,0.0), blue_diffuse));
    objects.push_back(new Plane(glm::vec3(0,27,0), glm::vec3(0.0,-1,0)));
    objects.push_back(new Plane(glm::vec3(0,1,-0.01), glm::vec3(0.0,0.0,1.0), green_diffuse));
	
	
	
	// Cones
	Material yellow_specular;
	yellow_specular.ambient = glm::vec3(0.1f, 0.10f, 0.0f);
	yellow_specular.diffuse = glm::vec3(0.4f, 0.4f, 0.0f);
	yellow_specular.specular = glm::vec3(1.0);
	yellow_specular.shininess = 100.0;
	
	Cone *cone = new Cone(yellow_specular);
	glm::mat4 translationMatrix = glm::translate(glm::vec3(5,9,14));
	glm::mat4 scalingMatrix = glm::scale(glm::vec3(3.0f, 12.0f, 3.0f));
	glm::mat4 rotationMatrix = glm::rotate(glm::radians(180.0f) , glm::vec3(1,0,0));
	cone->setTransformation(translationMatrix*scalingMatrix*rotationMatrix);
	objects.push_back(cone);
	
	Cone *cone2 = new Cone(green_diffuse);
	translationMatrix = glm::translate(glm::vec3(6,-3,7));
	scalingMatrix = glm::scale(glm::vec3(1.0f, 3.0f, 1.0f));
	rotationMatrix = glm::rotate(glm::atan(3.0f), glm::vec3(0,0,1));
	cone2->setTransformation(translationMatrix* rotationMatrix*scalingMatrix);
	objects.push_back(cone2);
	
}
glm::vec3 toneMapping(glm::vec3 intensity){
	float gamma = 1.0/2.0;
	float alpha = 12.0f;
	return glm::clamp(alpha * glm::pow(intensity, glm::vec3(gamma)), glm::vec3(0.0), glm::vec3(1.0));
}



int main(int argc, const char * argv[]) {

    clock_t render_ticks = clock(); // Tracks rendering time.

    int width = 1024; //width of the image
    int height = 768; // height of the image
    float field_of_view = 90; // field of view

	sceneDefinition(); // Set up scene objects and lights.

	Image image(width,height); // Create an image where we will store the result
	vector<glm::vec3> image_values(width*height);

    float pixel_size = 2*tan(0.5*field_of_view/180*M_PI)/width;
    float image_left = -pixel_size * width / 2;
    float image_top = pixel_size * height / 2;

    for(int pixel_x = 0; pixel_x < width ; pixel_x++)
        for(int pixel_y = 0; pixel_y < height ; pixel_y++){

			// Aim each ray through the center of its pixel.
			float ray_x = image_left + pixel_x*pixel_size + pixel_size/2;
            float ray_y = image_top - pixel_y*pixel_size - pixel_size/2;
            float ray_z = 1;

			glm::vec3 camera_origin(0, 0, 0);
            glm::vec3 direction(ray_x, ray_y, ray_z);
            direction = glm::normalize(direction);

            Ray ray(camera_origin, direction);
            image.setPixel(pixel_x, pixel_y, toneMapping(trace_ray(ray, 0, 0)));
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
