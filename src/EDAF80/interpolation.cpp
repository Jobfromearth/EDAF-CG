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

	//// === Catmull-Rom �������� ===
	//// ע�⣺GLM ��������M[col] ��ʾ�� col ��
	//glm::mat4 M;
	//M[0] = glm::vec4(0.0f, -t, 2.0f * t, -t); // column0: ��Ӧ p0 ϵ��
	//M[1] = glm::vec4(1.0f, 0.0f, t - 3.0f, 2.0f - t); // column1: ��Ӧ p1 ϵ��
	//M[2] = glm::vec4(0.0f, t, 3.0f - 2.0f * t, t - 2.0f); // column2: ��Ӧ p2 ϵ��
	//M[3] = glm::vec4(0.0f, 0.0f, -t, t); // column 3: ��Ӧ p3 ϵ��

	//// === control point matrix P (3row*4column��GLM mat4x3��4column��3raw) ===
	//glm::mat4x3 P;
	//P[0] = glm::vec3(p0); //  column 0 = p0 (x,y,z)
	//P[1] = glm::vec3(p1); //  column 1 = p1
	//P[2] = glm::vec3(p2); //  column 2= p2
	//P[3] = glm::vec3(p3); //  column 3 = p3

	//// === ���ս�� ===
	//// ���� M*X �õ�һ�� vec4��Ȼ�� P*(M*X) �� vec3
	//return P * (M * X);
	
	// ============expansion equation=======	
	glm::vec3 a0 = p1;
	glm::vec3 a1 = -t * p0 + t * p2;
	glm::vec3 a2 = 2.0f * t * p0 + (t - 3.0f) * p1 + (3.0f - 2.0f * t) * p2 - t * p3;
	glm::vec3 a3 = -t * p0 + (2.0f - t) * p1 + (t - 2.0f) * p2 + t * p3;

	// Horner��(((a3*x)+a2)*x + a1)*x + a0
	return ((a3 * x + a2) * x + a1) * x + a0;
}
