#include <iostream>
#include <string>
#include <assert.h>

using namespace std;
// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

#include <math.h>

// Shaders
const char* vertexShaderSource = "#version 460 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aColor;\n"
"out vec3 ourColor;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos, 1.0);\n"
"   ourColor = aColor;\n"
"}\0";

const char* fragmentShaderSource = "#version 460 core\n"
"in vec3 ourColor;\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(ourColor, 1.0);\n"
"}\0";


void processInput(GLFWwindow *window)
{
    // Fecha a janela quando ESC é pressionado
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}


int main() {
    // Inicializa a GLFW
    if (!glfwInit()) {
        std::cout << "Falha ao inicializar GLFW" << std::endl;
        return -1;
    }

    // Configuração de contexto OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Para macOS, descomente esta linha:
    // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // Cria a janela
    GLFWwindow* window = glfwCreateWindow(800, 600, "FraKk's cool Window", NULL, NULL);
    if (!window) {
        std::cout << "Falha ao criar janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    // Torna o contexto da janela como o contexto atual
    glfwMakeContextCurrent(window);

    // Inicializa o GLAD para carregar as funções OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Falha ao inicializar GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Define o viewport
    glViewport(0, 0, 800, 600);

    // Função de callback para redimensionamento da janela
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
        glViewport(0, 0, width, height);
    });

    // Loop principal
    while (!glfwWindowShouldClose(window)) {
        // Processa entrada
        processInput(window);

        // Renderização
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);  // Cor de fundo
        glClear(GL_COLOR_BUFFER_BIT);

        // Troca os buffers e verifica eventos
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Limpa recursos alocados
    glfwTerminate();
    return 0;
}