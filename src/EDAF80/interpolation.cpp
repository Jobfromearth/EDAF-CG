#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
	//! \todo Implement this function
	return (1.0f - x) * p0 + x * p1;
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0, glm::vec3 const& p1,
                              glm::vec3 const& p2, glm::vec3 const& p3,
                              float const t, float const x)
{
	////! \todo Implement this function
	// // Matrix form
	//glm::vec4 X(1.0f, x, x * x, x * x * x);

	//// === Catmull-Rom 基础矩阵 ===
	//// 注意：GLM 是列主序，M[col] 表示第 col 列
	//glm::mat4 M;
	//M[0] = glm::vec4(0.0f, -t, 2.0f * t, -t); // column0: 对应 p0 系数
	//M[1] = glm::vec4(1.0f, 0.0f, t - 3.0f, 2.0f - t); // column1: 对应 p1 系数
	//M[2] = glm::vec4(0.0f, t, 3.0f - 2.0f * t, t - 2.0f); // column2: 对应 p2 系数
	//M[3] = glm::vec4(0.0f, 0.0f, -t, t); // column 3: 对应 p3 系数

	//// === control point matrix P (3row*4column，GLM mat4x3：4column、3raw) ===
	//glm::mat4x3 P;
	//P[0] = glm::vec3(p0); //  column 0 = p0 (x,y,z)
	//P[1] = glm::vec3(p1); //  column 1 = p1
	//P[2] = glm::vec3(p2); //  column 2= p2
	//P[3] = glm::vec3(p3); //  column 3 = p3

	//// === 最终结果 ===
	//// 先算 M*X 得到一个 vec4，然后 P*(M*X) → vec3
	//return P * (M * X);
	
	// ============expansion equation=======	
	glm::vec3 a0 = p1;
	glm::vec3 a1 = -t * p0 + t * p2;
	glm::vec3 a2 = 2.0f * t * p0 + (t - 3.0f) * p1 + (3.0f - 2.0f * t) * p2 - t * p3;
	glm::vec3 a3 = -t * p0 + (2.0f - t) * p1 + (t - 2.0f) * p2 + t * p3;

	// Horner：(((a3*x)+a2)*x + a1)*x + a0
	return ((a3 * x + a2) * x + a1) * x + a0;
}
