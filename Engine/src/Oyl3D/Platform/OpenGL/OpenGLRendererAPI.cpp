#include "oylpch.h"
#include "OpenGLRendererAPI.h"

#include <glad/glad.h>

namespace oyl {

void OpenGLRendererAPI::setClearColor(float r, float g, float b, float a) {
	glClearColor(r, g, b, a);
}

void OpenGLRendererAPI::clear() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRendererAPI::drawIndexed(const std::shared_ptr<VertexArray>& vao) {
	glDrawElements(GL_TRIANGLES, vao->getIndexBuffer()->getCount(), GL_UNSIGNED_INT, nullptr);
}

}

