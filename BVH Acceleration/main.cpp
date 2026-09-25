/**
@file main.cpp
*/

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <ctime>
#include <vector>
#include "glm/common.hpp"
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


class Triangle : public Object {
	public:
		glm::vec3 vertex_1;
		glm::vec3 vertex_2;
		glm::vec3 vertex_3;
		glm::vec3 vertex_normal_1;
		glm::vec3 vertex_normal_2;
		glm::vec3 vertex_normal_3;


		Triangle(glm::vec3 vertex_1, glm::vec3 vertex_2, glm::vec3 vertex_3, glm::vec3 normal_at_vertex_1, glm::vec3 normal_at_vertex_2, glm::vec3 normal_at_vertex_3, Material material) {
			this->vertex_1 = vertex_1;
			this->vertex_2 = vertex_2;
			this->vertex_3 = vertex_3;


			this->vertex_normal_1 = normal_at_vertex_1;
			this->vertex_normal_2 = normal_at_vertex_2;
			this->vertex_normal_3 = normal_at_vertex_3;

			setMaterial(material);
		}

		Triangle(glm::vec3 vertex_1, glm::vec3 vertex_2, glm::vec3 vertex_3) {
			this->vertex_1 = vertex_1;
			this->vertex_2 = vertex_2;
			this->vertex_3 = vertex_3;

			// Zero normals mark triangles that need a face normal.
			this->vertex_normal_1 = glm::vec3(0.0f, 0.0f, 0.0f);
			this->vertex_normal_2 = glm::vec3(0.0f, 0.0f, 0.0f);
			this->vertex_normal_3 = glm::vec3(0.0f, 0.0f, 0.0f);
		}


		Hit intersect(Ray ray) {
			Hit hit;
			hit.hit = false;
			

			glm::vec3 plane_normal = glm::normalize(glm::cross(vertex_2 - vertex_1, vertex_3 - vertex_1));
			Plane plane = Plane(vertex_1, plane_normal);
			Hit plane_hit = plane.intersect(ray);


			if(plane_hit.hit == false) {
				return hit;
			}
			glm::vec3 intersection_point = plane_hit.intersection;
			
			
			glm::vec3 triangle_area_normal = glm::cross(vertex_2 - vertex_1, vertex_3 - vertex_1);
			glm::vec3 subtriangle_normal_1 = glm::cross(vertex_2 - intersection_point, vertex_3 - intersection_point);
			glm::vec3 subtriangle_normal_2 = glm::cross(vertex_3 - intersection_point, vertex_1 - intersection_point);
			glm::vec3 subtriangle_normal_3 = glm::cross(vertex_1 - intersection_point, vertex_2 - intersection_point);

			// The three barycentric weights locate the hit within the triangle.
			float weight_3 = glm::dot(triangle_area_normal, subtriangle_normal_3) / glm::dot(triangle_area_normal, triangle_area_normal);

			float weight_1 = glm::dot(triangle_area_normal, subtriangle_normal_1) / glm::dot(triangle_area_normal, triangle_area_normal);

			float weight_2 = glm::dot(triangle_area_normal, subtriangle_normal_2) / glm::dot(triangle_area_normal, triangle_area_normal);
			
			if(weight_1 >= 0.0f && weight_2 >= 0.0f && weight_3 >= 0.0f && weight_1 <= 1.0f && weight_2 <= 1.0f && weight_3 <= 1.0f) {
				hit.hit = true;
				hit.intersection = intersection_point;
				hit.distance = plane_hit.distance;
				// Use face normals when the mesh has no vertex normals.
				if(vertex_normal_1 == glm::vec3(0.0f, 0.0f, 0.0f)) {
					vertex_normal_1 = glm::normalize(glm::cross(vertex_2 - vertex_1, vertex_3 - vertex_1));
					vertex_normal_2 = glm::normalize(glm::cross(vertex_3 - vertex_2, vertex_1 - vertex_2));
					vertex_normal_3 = glm::normalize(glm::cross(vertex_1 - vertex_3, vertex_2 - vertex_3));
				}
				// Blend the vertex normals using the same weights.
				glm::vec3 interpolated_normal = weight_1 * vertex_normal_1 + weight_2 * vertex_normal_2 + weight_3 * vertex_normal_3;
				interpolated_normal = glm::normalize(interpolated_normal);
				hit.normal = interpolated_normal;
				hit.object = this;
			}
			
			return hit;
		}

		glm::vec3 get_center() {
			return (vertex_1 + vertex_2 + vertex_3) / 3.0f;
		}
}; 


class AABB : public Object {
	public:
		glm::vec3 min;
		glm::vec3 max;
		glm::vec3 center;

		AABB() {
			min = glm::vec3(INFINITY);
			max = glm::vec3(-INFINITY);
			center = glm::vec3(0.0f);
		}

		void include_vec_3(glm::vec3 point) {
			min.x = glm::min(point.x, min.x);
			min.y = glm::min(point.y, min.y);
			min.z = glm::min(point.z, min.z);
			max.x = glm::max(point.x, max.x);
			max.y = glm::max(point.y, max.y);
			max.z = glm::max(point.z, max.z);
			center = (min + max) * 0.5f;
		}

		void include_triangle(Triangle triangle) {
			include_vec_3(triangle.vertex_1);
			include_vec_3(triangle.vertex_2);
			include_vec_3(triangle.vertex_3);
		}


		Hit intersect(Ray ray) {
			Hit hit;
			hit.hit = false;
			float interval_near = -INFINITY;
			float interval_far = INFINITY;
			// Each axis narrows the interval where the ray is inside the box.
			for(int axis_index = 0; axis_index < 3; axis_index++) {
				float first_distance = (min[axis_index] - ray.origin[axis_index]) / ray.direction[axis_index];
				float second_distance = (max[axis_index] - ray.origin[axis_index]) / ray.direction[axis_index];
				float axis_near = glm::min(first_distance, second_distance);
				float axis_far = glm::max(first_distance, second_distance);
				interval_near = glm::max(interval_near, axis_near);
				interval_far = glm::min(interval_far, axis_far);
			}

			if(interval_near < interval_far ) {
				if(interval_far > 0.0f) {
					if(interval_near > interval_far) {
						return hit;
					}
					float distance = 0.0f;
					if(interval_near > 0.0f) {
						distance = interval_near;
					} else {
						distance = interval_far;
					}
					hit.hit = true;
					hit.distance = distance;
					hit.object = this;
					hit.normal = glm::vec3(0.0f);
					hit.intersection = ray.origin + distance * ray.direction;
					return hit;
				}
			}
			return hit;
		}
};



class Node {
	public:
		AABB box;
		vector<Triangle> triangles;
		Node* child_left;
		Node* child_right;

		Node() {
			child_left = nullptr;
			child_right = nullptr;
		}
};


void split(Node* parent, int depth);

// Build the root box before splitting the triangle list.
Node init_bhv(vector<Triangle> triangles) {
	AABB box = AABB(); 
	for(int triangle_index = 0; triangle_index < triangles.size(); triangle_index++) {
		Triangle triangle = triangles[triangle_index];
		box.include_triangle(triangle);
	}
	Node root = Node();
	root.box = box;
	root.triangles = triangles;
	split(&root, 0);
	return root;
}

void split(Node* parent, int depth) {
	// Stop at the depth limit or when only a few triangles remain.
	if(depth >= 40 || parent->triangles.size() < 4) {
		return;
	}

	Node* left_child = new Node();
	Node* right_child = new Node();

	// Cycle through x, y, and z when choosing the split axis.
	int axis_index = depth % 3;

	for(int triangle_index = 0; triangle_index < parent->triangles.size(); triangle_index++) {
		Triangle triangle = parent->triangles[triangle_index];
		if(triangle.get_center()[axis_index] < parent->box.center[axis_index]) {
			left_child->triangles.push_back(triangle);
			left_child->box.include_triangle(triangle);
		} else {
			right_child->triangles.push_back(triangle);
			right_child->box.include_triangle(triangle);
		}
	}

	parent->child_left = left_child;
	parent->child_right = right_child;
	split(parent->child_left, depth + 1);
	split(parent->child_right, depth + 1);
}

Hit traverse(Ray ray, Node* node, Hit best_hit) {
	// Skip subtrees whose bounding box is not hit by the ray.
	Hit box_hit = node->box.intersect(ray);
	if(box_hit.hit == true) {
		// Check triangles directly when this node is a leaf.
		if(node->child_left == nullptr && node->child_right == nullptr) {
			float best_distance = INFINITY;
			for(int triangle_index = 0; triangle_index < node->triangles.size(); triangle_index++) {
				Hit triangle_hit = node->triangles[triangle_index].intersect(ray);
				if(triangle_hit.hit == true && triangle_hit.distance < best_distance) {
					best_distance = triangle_hit.distance;
					best_hit = triangle_hit;
				}
			}
			return best_hit;
		} else {
			best_hit = traverse(ray, node->child_left, best_hit);
			best_hit = traverse(ray, node->child_right, best_hit);
			return best_hit;
		}
	}
	return best_hit;
}
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
vector<Triangle> triangles;
Node bvh_root;



/** Function for computing color of an object according to the Phong Model
 @param point A point belonging to the object for which the color is computed
 @param normal A normal vector to the point
 @param view_direction A normalized direction from the point to the viewer/camera
 @param material A material structure representing the material of the object
*/

int calc_light_s(Light* light, glm::vec3 point, glm::vec3 normal) {
	glm::vec3 start = point + normal * 0.001f;
	glm::vec3 light_direction = glm::normalize(light->position - start);
	float max_distance = glm::distance(light->position, start);
	Ray ray_from_point_to_light(start, light_direction);

	Hit best_hit;
	best_hit.hit = false;
	best_hit.distance = INFINITY;
	Hit triangle_hit = traverse(ray_from_point_to_light, &bvh_root, best_hit);
	if(triangle_hit.hit && triangle_hit.distance > 0.0001f && triangle_hit.distance < max_distance) {
		return 0;
	}

	return 1;
}

glm::vec3 PhongModel(glm::vec3 point, glm::vec3 normal, glm::vec3 view_direction, Material material){

	glm::vec3 color(0.0);
	for(int light_index = 0; light_index < lights.size(); light_index++){

		glm::vec3 light_direction = glm::normalize(lights[light_index]->position - point);
		glm::vec3 reflected_direction = glm::reflect(-light_direction, normal);

		float normal_dot_light = glm::clamp(glm::dot(normal, light_direction), 0.0f, 1.0f);
		float view_dot_reflection = glm::clamp(glm::dot(view_direction, reflected_direction), 0.0f, 1.0f);

		glm::vec3 diffuse = material.diffuse * glm::vec3(normal_dot_light);
		glm::vec3 specular = material.specular * glm::vec3(pow(view_dot_reflection, material.shininess));
		
        float light_distance = glm::distance(point,lights[light_index]->position);
        light_distance = max(light_distance, 0.1f);
		//int light_visible = calc_light_s(lights[light_num], point, normal);
        color +=  (lights[light_index]->color * (diffuse + specular)) / light_distance/light_distance;
	}
	color += ambient_light * material.ambient;
	color = glm::clamp(color, glm::vec3(0.0), glm::vec3(1.0));
	return color;
}

// Rotate a vector around the Y axis by angle radians
glm::vec3 rotateY(const glm::vec3& vector, float angle_rad){
	float cos_angle = cosf(angle_rad);
	float sin_angle = sinf(angle_rad);
	return glm::vec3(cos_angle * vector.x + sin_angle * vector.z, vector.y, -sin_angle * vector.x + cos_angle * vector.z);
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
	closest_hit = traverse(ray, &bvh_root, closest_hit);

	if(closest_hit.hit == false) {
		for(size_t object_index = 0; object_index < objects.size(); object_index++){
			Hit hit = objects[object_index]->intersect(ray);
			if(hit.hit == true && hit.distance < closest_hit.distance){
				closest_hit = hit;
			}
		}
	}
		

	glm::vec3 color(0.0);
	if(closest_hit.hit){
		color = PhongModel(closest_hit.intersection, closest_hit.normal, glm::normalize(-ray.direction), closest_hit.object->getMaterial());
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
	

	Reader reader_bunny = Reader("/Users/alexanderschramm/Downloads/assignment 3 cg/code/meshes/bunny.obj", false);
	Reader reader_armadillo = Reader("/Users/alexanderschramm/Downloads/assignment 3 cg/code/meshes/armadillo.obj", false);
	Reader reader_lucy = Reader("/Users/alexanderschramm/Downloads/assignment 3 cg/code/meshes/lucy.obj", false);
	// dragon file has no usable normals; read indices only and rebuild smooth normals below
	Reader reader_dragon = Reader("/Users/alexanderschramm/Downloads/bhv_implementation/code/meshes/dragon.obj", false);

	bool render_bunny = false;
	bool render_armadillo = false;
	bool render_lucy = false;
	bool render_dragon = true;
	glm::vec3 translate_bunny = glm::vec3(0.0f, -3.0f, 8.0f);
	glm::vec3 translate_armadillo = glm::vec3(-4.0f, -3.0f, 10.0f);
	glm::vec3 translate_lucy = glm::vec3(4.0f, -3.0f, 10.0f);
	glm::vec3 translate_dragon = glm::vec3(0.0f, -3.0f, 25.0f);
	glm::vec3 scale_dragon = glm::vec3(15.0f, 15.0f, 15.0f);
	float rotate_dragon_y_deg = 50.0f; // adjust to desired rotation
	float rotate_dragon_y_rad = glm::radians(rotate_dragon_y_deg);


	if(render_bunny) {
	
		for(size_t face_index = 0; face_index < reader_bunny.vertex_indices.size(); face_index++) {
			int vertex_index_1 = reader_bunny.vertex_indices[face_index].x;
			int vertex_index_2 = reader_bunny.vertex_indices[face_index].y;
			int vertex_index_3 = reader_bunny.vertex_indices[face_index].z;
			
			glm::vec3 vertex_1 = reader_bunny.vertices[vertex_index_1] + translate_bunny;
			glm::vec3 vertex_2 = reader_bunny.vertices[vertex_index_2] + translate_bunny;
			glm::vec3 vertex_3 = reader_bunny.vertices[vertex_index_3] + translate_bunny;



			Triangle triangle = Triangle(vertex_1, vertex_2, vertex_3);
			triangles.push_back(triangle);
		}
	}

	if(render_armadillo) {
		for(size_t face_index = 0; face_index < reader_armadillo.vertex_indices.size(); face_index++) {
			int vertex_index_1 = reader_armadillo.vertex_indices[face_index].x;
			int vertex_index_2 = reader_armadillo.vertex_indices[face_index].y;
			int vertex_index_3 = reader_armadillo.vertex_indices[face_index].z;
			
			glm::vec3 vertex_1 = reader_armadillo.vertices[vertex_index_1] + translate_armadillo;
			glm::vec3 vertex_2 = reader_armadillo.vertices[vertex_index_2] + translate_armadillo;
			glm::vec3 vertex_3 = reader_armadillo.vertices[vertex_index_3] + translate_armadillo;

			Triangle triangle = Triangle(vertex_1, vertex_2, vertex_3);
			triangles.push_back(triangle);
		}
	}

	if(render_lucy) {
		for(size_t face_index = 0; face_index < reader_lucy.vertex_indices.size(); face_index++) {
			int vertex_index_1 = reader_lucy.vertex_indices[face_index].x;
			int vertex_index_2 = reader_lucy.vertex_indices[face_index].y;
			int vertex_index_3 = reader_lucy.vertex_indices[face_index].z;
			
			glm::vec3 vertex_1 = reader_lucy.vertices[vertex_index_1] + translate_lucy;
			glm::vec3 vertex_2 = reader_lucy.vertices[vertex_index_2] + translate_lucy;
			glm::vec3 vertex_3 = reader_lucy.vertices[vertex_index_3] + translate_lucy;

			Triangle triangle = Triangle(vertex_1, vertex_2, vertex_3);
			triangles.push_back(triangle);
		}
	}

	if(render_dragon) {
		// Smooth normals: compute on transformed vertices
		std::vector<glm::vec3> transformed_vertices(reader_dragon.vertices.size());
		for(size_t vertex_index = 0; vertex_index < reader_dragon.vertices.size(); ++vertex_index){
			transformed_vertices[vertex_index] = rotateY(reader_dragon.vertices[vertex_index] * scale_dragon, rotate_dragon_y_rad) + translate_dragon;
		}
		// Average the neighboring face normals to shade the dragon smoothly.
		std::vector<glm::vec3> accumulated_normals(reader_dragon.vertices.size(), glm::vec3(0.0f));
		for(size_t face_index = 0; face_index < reader_dragon.vertex_indices.size(); face_index++) {
			int vertex_index_1 = reader_dragon.vertex_indices[face_index].x;
			int vertex_index_2 = reader_dragon.vertex_indices[face_index].y;
			int vertex_index_3 = reader_dragon.vertex_indices[face_index].z;

			glm::vec3 vertex_1 = transformed_vertices[vertex_index_1];
			glm::vec3 vertex_2 = transformed_vertices[vertex_index_2];
			glm::vec3 vertex_3 = transformed_vertices[vertex_index_3];

			glm::vec3 face_normal = glm::normalize(glm::cross(vertex_2 - vertex_1, vertex_3 - vertex_1));
			accumulated_normals[vertex_index_1] += face_normal;
			accumulated_normals[vertex_index_2] += face_normal;
			accumulated_normals[vertex_index_3] += face_normal;
		}
		for(size_t vertex_index = 0; vertex_index < accumulated_normals.size(); ++vertex_index) {
			if(glm::length(accumulated_normals[vertex_index]) > 0.0f) {
				accumulated_normals[vertex_index] = glm::normalize(accumulated_normals[vertex_index]);
			}
		}

		for(size_t face_index = 0; face_index < reader_dragon.vertex_indices.size(); face_index++) {
			int vertex_index_1 = reader_dragon.vertex_indices[face_index].x;
			int vertex_index_2 = reader_dragon.vertex_indices[face_index].y;
			int vertex_index_3 = reader_dragon.vertex_indices[face_index].z;
			
			glm::vec3 vertex_1 = transformed_vertices[vertex_index_1];
			glm::vec3 vertex_2 = transformed_vertices[vertex_index_2];
			
			glm::vec3 vertex_3 = transformed_vertices[vertex_index_3];

			glm::vec3 normal_1 = accumulated_normals[vertex_index_1];
			glm::vec3 normal_2 = accumulated_normals[vertex_index_2];
			glm::vec3 normal_3 = accumulated_normals[vertex_index_3];

			Triangle triangle = Triangle(vertex_1, vertex_2, vertex_3, normal_1, normal_2, normal_3, red_specular);
			triangles.push_back(triangle);
		}
	}

	cout << "Triangles: " << triangles.size() << endl;
	
	
	bvh_root = init_bhv(triangles);
	
}

glm::vec3 toneMapping(glm::vec3 intensity){
	float gamma = 0.5f;
	float alpha = 12.0f;
	return glm::clamp(alpha * glm::pow(intensity, glm::vec3(gamma)), glm::vec3(0.0), glm::vec3(1.0));
}

int main(int argc, char * argv[]) {

    clock_t render_ticks = clock();

    int width = 1024;
    int height = 768;
    float field_of_view = 90;

	sceneDefinition();

    Image image(width,height);
    vector<glm::vec3> image_values(width*height);

	size_t total_pixels = static_cast<size_t>(width) * static_cast<size_t>(height);
	size_t processed_pixels = 0;
	int last_printed_hundredths = -1; // track progress in 0.01% steps
	std::cout.setf(std::ios::unitbuf); // auto-flush stdout to show progress promptly
	std::cout << std::fixed << std::setprecision(2);
	std::cout << "Rendering: 0.00% completed" << std::endl;

    float pixel_size = 2*tan(0.5*field_of_view/180*M_PI)/width;
    float image_left = -pixel_size * width / 2;
    float image_top = pixel_size * height / 2;

    for(int pixel_x = 0; pixel_x < width ; pixel_x++) {
        for(int pixel_y = 0; pixel_y < height ; pixel_y++){
			glm::vec3 color_sum(0.0f);
			// 2x2 supersampling (simple grid)
			for(int sample_x = 0; sample_x < 2; ++sample_x){
				for(int sample_y = 0; sample_y < 2; ++sample_y){
					float ray_x = image_left + (pixel_x + (sample_x + 0.5f)/2.0f) * pixel_size;
					float ray_y = image_top - (pixel_y + (sample_y + 0.5f)/2.0f) * pixel_size;
					float ray_z = 1.0f;

					glm::vec3 camera_origin(0, 0, 0);
					glm::vec3 direction(ray_x, ray_y, ray_z);
					direction = glm::normalize(direction);

					Ray ray(camera_origin, direction);
					color_sum += toneMapping(trace_ray(ray));
				}
			}
			glm::vec3 final_color = color_sum * 0.25f; // average of 4 samples
            image.setPixel(pixel_x, pixel_y, final_color);
			
			processed_pixels++;
			double percent = (static_cast<double>(processed_pixels) * 100.0) / static_cast<double>(total_pixels);
			int percent_hundredths = static_cast<int>(percent * 100); // e.g., 12.34% -> 1234
			if(percent_hundredths > last_printed_hundredths){
				cout << "Rendering: " << percent << "% completed" << endl;
				last_printed_hundredths = percent_hundredths;
			}
        }
	}
	cout << "Rendering: 100.00% completed" << endl;
	
    render_ticks = clock() - render_ticks;
	cout<<"It took " << ((float)render_ticks)/CLOCKS_PER_SEC<< " seconds to render the image."<< endl;
    cout<<"I could render at "<< (float)CLOCKS_PER_SEC/((float)render_ticks) << " frames per second."<<endl;

	// Save the image to the requested path, or use result.ppm.
	if (argc == 2){
		image.writeImage(argv[1]);
		std::cout << "Image written to " << argv[1] << std::endl;
	}else{
		image.writeImage("./result.ppm");
		std::cout << "Image written to ./result.ppm" << std::endl;
	}
	
    return 0;
}

