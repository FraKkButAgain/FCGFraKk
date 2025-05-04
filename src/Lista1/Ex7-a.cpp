#include <iostream>
#include <string>
#include <vector>
#include <assert.h>

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

#include <math.h>

const float steps = 8;
const float angle = 3.1415926f * 2.0f / steps;

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
    
    if (!glfwInit()) {
        std::cout << "Falha ao inicializar GLFW" << std::endl;
        return -1;
    }

    // Configuração de contexto OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    


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

    // Compilar o vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // Verificar erros de compilação
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERRO::SHADER::VERTEX::COMPILACAO_FALHOU\n" << infoLog << std::endl;
    }

    // Compilar o fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // Verificar erros de compilação
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERRO::SHADER::FRAGMENT::COMPILACAO_FALHOU\n" << infoLog << std::endl;
    }

    // Linkar os shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // Verificar erros de linkagem
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERRO::PROGRAMA::LINKAGEM_FALHOU\n" << infoLog << std::endl;
    }
    
    // Liberar shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Preparar dados do círculo
    float radius = 0.5f;
    std::vector<float> vertices;
    
    // Vértice central
    vertices.push_back(0.0f);  // posição x
    vertices.push_back(0.0f);  // posição y
    vertices.push_back(0.0f);  // posição z
    vertices.push_back(0.5f);  // cor r
    vertices.push_back(0.0f);  // cor g
    vertices.push_back(0.0f);  // cor b
    
    // Primeiro ponto do círculo
    float firstX = radius * sin(0);
    float firstY = -radius * cos(0);
    
    for (int i = 0; i <= steps; i++) {
        float x = radius * sin(angle * i);
        float y = -radius * cos(angle * i);
        
        vertices.push_back(x);     // posição x
        vertices.push_back(y);     // posição y
        vertices.push_back(0.0f);  // posição z
        vertices.push_back(0.5f);  // cor r
        vertices.push_back(0.0f);  // cor g
        vertices.push_back(0.0f);  // cor b
    }
    
    // Criar índices para os triângulos
    std::vector<unsigned int> indices;
    for (int i = 1; i <= steps; i++) {
        indices.push_back(0);  // Vértice central
        indices.push_back(i);  // Vértice atual
        indices.push_back(i + 1); // Próximo vértice
    }
    
    // Fechar o círculo
    indices.push_back(0);
    indices.push_back(steps);
    indices.push_back(1);
    
    // Configurar VAO, VBO e EBO
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Posição dos atributos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Cor dos atributos
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Loop principal
    while (!glfwWindowShouldClose(window)) {
        // Processa entrada
        processInput(window);
        
        // Renderização
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);  // Cor de fundo
        glClear(GL_COLOR_BUFFER_BIT);

        // Usar o shader program
        glUseProgram(shaderProgram);
        
        // Desenhar o círculo
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        
        // Troca os buffers e verifica eventos
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Limpar recursos
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    
    // Limpa recursos alocados
    glfwTerminate();
    return 0;
}