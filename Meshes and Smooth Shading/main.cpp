/**
@file main.cpp
*/

#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>
#include <vector>
#include "glm/ext/vector_float3.hpp"
#include "my_obj_reader.cpp"
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
        }
		else{
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
