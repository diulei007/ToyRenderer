#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>


#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "lib/camera.h"
#include "lib/shader.h"

#define INF 111111.0

using namespace glm;

unsigned int SCR_WIDTH = 1200;
unsigned int SCR_HEIGHT = 900;

Camera camera(vec3(0, 0, 4));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;



bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

GLuint program;                 // ��ɫ���������
std::vector<vec3> vertices;     // ��������
std::vector<GLuint> indices;    // ��������
std::vector<vec3> lines;        // �߶ζ˵�����

struct BVHNode {
	BVHNode* left;
	BVHNode* right;
	int n, index;
	vec3 AA, BB;
};

struct Triangle {
	vec3 p1, p2, p3;
	vec3 center;
	Triangle(vec3 a, vec3 b, vec3 c) {
		p1 = a, p2 = b, p3 = c;
		center = (p1 + p2 + p3) / vec3(3, 3, 3);
	}
};

std::vector<Triangle> triangles;

bool cmpx(const Triangle& t1, const Triangle& t2) {
	return t1.center.x < t2.center.x;
}
bool cmpy(const Triangle& t1, const Triangle& t2) {
	return t1.center.y < t2.center.y;
}
bool cmpz(const Triangle& t1, const Triangle& t2) {
	return t1.center.z < t2.center.z;
}

// �󽻽��
struct HitResult {
	Triangle* triangle = NULL;
	float distance = INF;
};

// ����
typedef struct Ray {
	vec3 startPoint = vec3(0, 0, 0);    // ���
	vec3 direction = vec3(0, 0, 0);     // ����
}Ray;

// ��ȡ obj
void readObj(std::string filepath, std::vector<vec3>& vertices, std::vector<GLuint>& indices) {
	// ���ļ���
	std::ifstream fin(filepath);
	std::string line;
	if (!fin.is_open()) {
		std::cout << "�ļ� " << filepath << " ��ʧ��" << std::endl;
		exit(-1);
	}

	// ������ȡ
	int offset = vertices.size();

	// ���ж�ȡ
	while (std::getline(fin, line)) {
		std::istringstream sin(line);   // ��һ�е�������Ϊ string stream �������Ҷ�ȡ
		std::string type;
		GLfloat x, y, z;
		int v0, v1, v2;

		// ��ȡobj�ļ�
		sin >> type;
		if (type == "v") {
			sin >> x >> y >> z;
			vertices.push_back(vec3(x, y, z));
		}
		if (type == "f") {
			sin >> v0 >> v1 >> v2;

			indices.push_back(v0 - 1 + offset);
			indices.push_back(v1 - 1 + offset);
			indices.push_back(v2 - 1 + offset);
		}
	}
}

void addLine(vec3 p1, vec3 p2) {
	lines.push_back(p1);
	lines.push_back(p2);
}

void addBox(BVHNode* root) {
	float x1 = root->AA.x, y1 = root->AA.y, z1 = root->AA.z;
	float x2 = root->BB.x, y2 = root->BB.y, z2 = root->BB.z;
	lines.push_back(vec3(x1, y1, z1)), lines.push_back(vec3(x2, y1, z1));
	lines.push_back(vec3(x1, y1, z1)), lines.push_back(vec3(x1, y1, z2));
	lines.push_back(vec3(x1, y1, z1)), lines.push_back(vec3(x1, y2, z1));
	lines.push_back(vec3(x2, y1, z1)), lines.push_back(vec3(x2, y1, z2));
	lines.push_back(vec3(x2, y1, z1)), lines.push_back(vec3(x2, y2, z1));
	lines.push_back(vec3(x1, y2, z1)), lines.push_back(vec3(x2, y2, z1));
	lines.push_back(vec3(x1, y1, z2)), lines.push_back(vec3(x1, y2, z2));
	lines.push_back(vec3(x1, y2, z1)), lines.push_back(vec3(x1, y2, z2));
	lines.push_back(vec3(x1, y2, z2)), lines.push_back(vec3(x2, y2, z2));
	lines.push_back(vec3(x1, y1, z2)), lines.push_back(vec3(x2, y1, z2));
	lines.push_back(vec3(x2, y2, z1)), lines.push_back(vec3(x2, y2, z2));
	lines.push_back(vec3(x2, y1, z2)), lines.push_back(vec3(x2, y2, z2));
}

void addTriangle(Triangle* tri) {
	if (tri) {
		lines.push_back(tri->p1 - vec3(0.0005, 0.0005, 0.0005));
		lines.push_back(tri->p2 - vec3(0.0005, 0.0005, 0.0005));
		lines.push_back(tri->p2 - vec3(0.0005, 0.0005, 0.0005));
		lines.push_back(tri->p3 - vec3(0.0005, 0.0005, 0.0005));
		lines.push_back(tri->p3 - vec3(0.0005, 0.0005, 0.0005));
		lines.push_back(tri->p1 - vec3(0.0005, 0.0005, 0.0005));
		lines.push_back(tri->p1 + vec3(0.0005, 0.0005, 0.0005));
		lines.push_back(tri->p2 + vec3(0.0005, 0.0005, 0.0005));
		lines.push_back(tri->p2 + vec3(0.0005, 0.0005, 0.0005));
		lines.push_back(tri->p3 + vec3(0.0005, 0.0005, 0.0005));
		lines.push_back(tri->p3 + vec3(0.0005, 0.0005, 0.0005));
		lines.push_back(tri->p1 + vec3(0.0005, 0.0005, 0.0005));
	}
}

// ���ߺ��������� 
float hitTriangle(Triangle* triangle, Ray ray) {

	vec3 p1 = triangle->p1, p2 = triangle->p2, p3 = triangle->p3;
	vec3 S = ray.startPoint;        // �������
	vec3 d = ray.direction;         // ���߷���
	vec3 N = normalize(cross(p2 - p1, p3 - p1));    // ������
	if (dot(N, d) > 0.0f) N = -N;   // ��ȡ��ȷ�ķ�����

	// ������ߺ�������ƽ��
	if (fabs(dot(N, d)) < 0.00001f) return INF;

	// ����
	float t = (dot(N, p1) - dot(S, N)) / dot(d, N);
	if (t < 0.0005f) return INF;    // ����������ڹ��߱���

	// �������
	vec3 P = S + d * t;

	// �жϽ����Ƿ�����������
	vec3 c1 = cross(p2 - p1, P - p1);
	vec3 c2 = cross(p3 - p2, P - p2);
	vec3 c3 = cross(p1 - p3, P - p3);
	if (dot(c1, N) > 0 && dot(c2, N) > 0 && dot(c3, N) > 0) return t;
	if (dot(c1, N) < 0 && dot(c2, N) < 0 && dot(c3, N) < 0) return t;

	return INF;
}

BVHNode* buildBVH(std::vector<Triangle>& triangles, int l, int r, int n) {
	if (l > r) return 0;

	BVHNode* node = new BVHNode();
	node->AA = vec3(1145141919, 1145141919, 1145141919);
	node->BB = vec3(-1145141919, -1145141919, -1145141919);

	// ���� AABB
	for (int i = l; i <= r; i++) {
		// ��С�� AA
		float minx = min(triangles[i].p1.x, min(triangles[i].p2.x, triangles[i].p3.x));
		float miny = min(triangles[i].p1.y, min(triangles[i].p2.y, triangles[i].p3.y));
		float minz = min(triangles[i].p1.z, min(triangles[i].p2.z, triangles[i].p3.z));
		node->AA.x = min(node->AA.x, minx);
		node->AA.y = min(node->AA.y, miny);
		node->AA.z = min(node->AA.z, minz);
		// ���� BB
		float maxx = max(triangles[i].p1.x, max(triangles[i].p2.x, triangles[i].p3.x));
		float maxy = max(triangles[i].p1.y, max(triangles[i].p2.y, triangles[i].p3.y));
		float maxz = max(triangles[i].p1.z, max(triangles[i].p2.z, triangles[i].p3.z));
		node->BB.x = max(node->BB.x, maxx);
		node->BB.y = max(node->BB.y, maxy);
		node->BB.z = max(node->BB.z, maxz);
	}

	// ������ n �������� ����Ҷ�ӽڵ�
	if ((r - l + 1) <= n) {
		node->n = r - l + 1;
		node->index = l;
		return node;
	}

	// ����ݹ齨��
	float lenx = node->BB.x - node->AA.x;
	float leny = node->BB.y - node->AA.y;
	float lenz = node->BB.z - node->AA.z;
	// �� x ����
	if (lenx >= leny && lenx >= lenz)
		std::sort(triangles.begin() + l, triangles.begin() + r + 1, cmpx);
	// �� y ����
	if (leny >= lenx && leny >= lenz)
		std::sort(triangles.begin() + l, triangles.begin() + r + 1, cmpy);
	// �� z ����
	if (lenz >= lenx && lenz >= leny)
		std::sort(triangles.begin() + l, triangles.begin() + r + 1, cmpz);

	// �ݹ�
	int mid = (l + r) / 2;
	node->left = buildBVH(triangles, l, mid, n);
	node->right = buildBVH(triangles, mid + 1, r, n);

	return node;
}

// SAH �Ż����� BVH
BVHNode* buildBVHwithSAH(std::vector<Triangle>& triangles, int l, int r, int n) {
	if (l > r) return 0;

	BVHNode* node = new BVHNode();
	node->AA = vec3(1145141919, 1145141919, 1145141919);
	node->BB = vec3(-1145141919, -1145141919, -1145141919);

	// ���� AABB
	for (int i = l; i <= r; i++) {
		// ��С�� AA
		float minx = min(triangles[i].p1.x, min(triangles[i].p2.x, triangles[i].p3.x));
		float miny = min(triangles[i].p1.y, min(triangles[i].p2.y, triangles[i].p3.y));
		float minz = min(triangles[i].p1.z, min(triangles[i].p2.z, triangles[i].p3.z));
		node->AA.x = min(node->AA.x, minx);
		node->AA.y = min(node->AA.y, miny);
		node->AA.z = min(node->AA.z, minz);
		// ���� BB
		float maxx = max(triangles[i].p1.x, max(triangles[i].p2.x, triangles[i].p3.x));
		float maxy = max(triangles[i].p1.y, max(triangles[i].p2.y, triangles[i].p3.y));
		float maxz = max(triangles[i].p1.z, max(triangles[i].p2.z, triangles[i].p3.z));
		node->BB.x = max(node->BB.x, maxx);
		node->BB.y = max(node->BB.y, maxy);
		node->BB.z = max(node->BB.z, maxz);
	}

	// ������ n �������� ����Ҷ�ӽڵ�
	if ((r - l + 1) <= n) {
		node->n = r - l + 1;
		node->index = l;
		return node;
	}

	// ����ݹ齨��
	float Cost = INF;
	int Axis = 0;
	int Split = (l + r) / 2;
	for (int axis = 0; axis < 3; axis++) {
		// �ֱ� x��y��z ������
		if (axis == 0) std::sort(&triangles[0] + l, &triangles[0] + r + 1, cmpx);
		if (axis == 1) std::sort(&triangles[0] + l, &triangles[0] + r + 1, cmpy);
		if (axis == 2) std::sort(&triangles[0] + l, &triangles[0] + r + 1, cmpz);

		// leftMax[i]: [l, i] ������ xyz ֵ
		// leftMin[i]: [l, i] ����С�� xyz ֵ
		std::vector<vec3> leftMax(r - l + 1, vec3(-INF, -INF, -INF));
		std::vector<vec3> leftMin(r - l + 1, vec3(INF, INF, INF));
		// ����ǰ׺ ע�� i-l �Զ��뵽�±� 0
		for (int i = l; i <= r; i++) {
			Triangle& t = triangles[i];
			int bias = (i == l) ? 0 : 1;  // ��һ��Ԫ�����⴦��

			leftMax[i - l].x = max(leftMax[i - l - bias].x, max(t.p1.x, max(t.p2.x, t.p3.x)));
			leftMax[i - l].y = max(leftMax[i - l - bias].y, max(t.p1.y, max(t.p2.y, t.p3.y)));
			leftMax[i - l].z = max(leftMax[i - l - bias].z, max(t.p1.z, max(t.p2.z, t.p3.z)));

			leftMin[i - l].x = min(leftMin[i - l - bias].x, min(t.p1.x, min(t.p2.x, t.p3.x)));
			leftMin[i - l].y = min(leftMin[i - l - bias].y, min(t.p1.y, min(t.p2.y, t.p3.y)));
			leftMin[i - l].z = min(leftMin[i - l - bias].z, min(t.p1.z, min(t.p2.z, t.p3.z)));
		}

		// rightMax[i]: [i, r] ������ xyz ֵ
		// rightMin[i]: [i, r] ����С�� xyz ֵ
		std::vector<vec3> rightMax(r - l + 1, vec3(-INF, -INF, -INF));
		std::vector<vec3> rightMin(r - l + 1, vec3(INF, INF, INF));
		// �����׺ ע�� i-l �Զ��뵽�±� 0
		for (int i = r; i >= l; i--) {
			Triangle& t = triangles[i];
			int bias = (i == r) ? 0 : 1;  // ��һ��Ԫ�����⴦��

			rightMax[i - l].x = max(rightMax[i - l + bias].x, max(t.p1.x, max(t.p2.x, t.p3.x)));
			rightMax[i - l].y = max(rightMax[i - l + bias].y, max(t.p1.y, max(t.p2.y, t.p3.y)));
			rightMax[i - l].z = max(rightMax[i - l + bias].z, max(t.p1.z, max(t.p2.z, t.p3.z)));

			rightMin[i - l].x = min(rightMin[i - l + bias].x, min(t.p1.x, min(t.p2.x, t.p3.x)));
			rightMin[i - l].y = min(rightMin[i - l + bias].y, min(t.p1.y, min(t.p2.y, t.p3.y)));
			rightMin[i - l].z = min(rightMin[i - l + bias].z, min(t.p1.z, min(t.p2.z, t.p3.z)));
		}

		// ����Ѱ�ҷָ�
		float cost = INF;
		int split = l;
		for (int i = l; i <= r - 1; i++) {
			float lenx, leny, lenz;
			// ��� [l, i]
			vec3 leftAA = leftMin[i - l];
			vec3 leftBB = leftMax[i - l];
			lenx = leftBB.x - leftAA.x;
			leny = leftBB.y - leftAA.y;
			lenz = leftBB.z - leftAA.z;
			float leftS = 2.0 * ((lenx * leny) + (lenx * lenz) + (leny * lenz));
			float leftCost = leftS * (i - l + 1);

			// �Ҳ� [i+1, r]
			vec3 rightAA = rightMin[i + 1 - l];
			vec3 rightBB = rightMax[i + 1 - l];
			lenx = rightBB.x - rightAA.x;
			leny = rightBB.y - rightAA.y;
			lenz = rightBB.z - rightAA.z;
			float rightS = 2.0 * ((lenx * leny) + (lenx * lenz) + (leny * lenz));
			float rightCost = rightS * (r - i);

			// ��¼ÿ���ָ����С��
			float totalCost = leftCost + rightCost;
			if (totalCost < cost) {
				cost = totalCost;
				split = i;
			}
		}
		// ��¼ÿ�������Ѵ�
		if (cost < Cost) {
			Cost = cost;
			Axis = axis;
			Split = split;
		}
	}

	// �������ָ�
	if (Axis == 0) std::sort(&triangles[0] + l, &triangles[0] + r + 1, cmpx);
	if (Axis == 1) std::sort(&triangles[0] + l, &triangles[0] + r + 1, cmpy);
	if (Axis == 2) std::sort(&triangles[0] + l, &triangles[0] + r + 1, cmpz);

	// �ݹ�
	node->left = buildBVHwithSAH(triangles, l, Split, n);
	node->right = buildBVHwithSAH(triangles, Split + 1, r, n);

	return node;
}

void dfsNlevel(BVHNode* root, int depth, int targetDepth) {
	if (root == NULL) return;
	if (targetDepth == depth) {
		addBox(root);
		return;
	}
	dfsNlevel(root->left, depth + 1, targetDepth);
	dfsNlevel(root->right, depth + 1, targetDepth);
}

// ����������
HitResult hitTriangleArray(Ray ray, std::vector<Triangle>& triangles, int l, int r) {
	HitResult res;
	for (int i = l; i <= r; i++) {
		float d = hitTriangle(&triangles[i], ray);
		if (d < INF && d < res.distance) {
			res.distance = d;
			res.triangle = &triangles[i];
		}
	}
	return res;
}

// �� aabb �����󽻣�û�н����򷵻� -1
float hitAABB(Ray r, vec3 AA, vec3 BB) {
	// 1.0 / direction
	vec3 invdir = vec3(1.0 / r.direction.x, 1.0 / r.direction.y, 1.0 / r.direction.z);

	vec3 in = (BB - r.startPoint) * invdir;
	vec3 out = (AA - r.startPoint) * invdir;

	vec3 tmax = max(in, out);
	vec3 tmin = min(in, out);

	float t1 = min(tmax.x, min(tmax.y, tmax.z));
	float t0 = max(tmin.x, max(tmin.y, tmin.z));

	return (t1 >= t0) ? ((t0 > 0.0) ? (t0) : (t1)) : (-1);
}

// �� BVH �ϱ�����
HitResult hitBVH(Ray ray, std::vector<Triangle>& triangles, BVHNode* root) {
	if (root == NULL) return HitResult();

	// ��Ҷ�� ������
	if (root->n > 0) {
		return hitTriangleArray(ray, triangles, root->n, root->n + root->index - 1);
	}

	// ���������� AABB ��
	float d1 = INF, d2 = INF;
	if (root->left) d1 = hitAABB(ray, root->left->AA, root->left->BB);
	if (root->right) d2 = hitAABB(ray, root->right->AA, root->right->BB);

	// �ݹ���
	HitResult r1, r2;
	if (d1 > 0) r1 = hitBVH(ray, triangles, root->left);
	if (d2 > 0) r2 = hitBVH(ray, triangles, root->right);

	return r1.distance < r2.distance ? r1 : r2;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		camera.ProcessKeyboard(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
		camera.ProcessKeyboard(DOWN, deltaTime);
}

int main()
{
	glfwInit();
	glfwInitHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwInitHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwInitHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "BVH", NULL, NULL);
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
	glEnable(GL_DEPTH_TEST);

	Shader rabbit("shaders/vshader.vert", "shaders/fshader.frag");

	readObj("models/Stanford Bunny.obj", vertices, indices);
	for (auto& v : vertices) {
		v.x *= 5.0, v.y *= 5.0, v.z *= 5.0;
		v.y -= 0.5;
	}
	readObj("models/quad.obj", vertices, indices);
	// ���� Triangle ����
	for (int i = 0; i < indices.size(); i += 3) {
		triangles.push_back(Triangle(vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]]));
	}

	// ���� BVH ��
	//BVHNode* root = buildBVH(triangles, 0, triangles.size() - 1, 8);
	BVHNode* root = buildBVHwithSAH(triangles, 0, triangles.size() - 1, 8);

	dfsNlevel(root, 0, 5); // ���ӻ��� n �� bvh

	Ray ray;
	ray.startPoint = vec3(0, 0, 1);
	ray.direction = normalize(vec3(0.1, -0.1, -0.7));

	// ������֤
	HitResult res = hitBVH(ray, triangles, root);
	addTriangle(res.triangle);
	addLine(ray.startPoint, ray.startPoint + ray.direction * vec3(5, 5, 5));

	// ����vao�����Ұ�vao
	// ����vbo�����Ұ�vbo
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * (vertices.size() + lines.size()), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3) * vertices.size(), vertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec3) * vertices.size(), sizeof(vec3) * lines.size(), lines.data());

	// ����vao�����Ұ�vao
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// �������� ebo
	GLuint ebo;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processInput(window);

		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 200.0f);
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));

		rabbit.use();
		rabbit.setMat4("projection", projection);
		rabbit.setMat4("view", view);
		rabbit.setMat4("model", model);
		rabbit.setVec3("color", vec3(1, 0, 0));
		glEnable(GL_DEPTH_TEST);
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);       // ��մ�����ɫ����
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		
		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
		

		rabbit.setVec3("color", vec3(1, 1, 1));
		glDrawArrays(GL_LINES, vertices.size(), lines.size());
		glBindVertexArray(0);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);
	glDeleteVertexArrays(1, &vao);
	glfwTerminate();
	return 0;
}