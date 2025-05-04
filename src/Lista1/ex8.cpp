#include <iostream>
#include <string>
#include <vector>
#include <assert.h>

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

#include <math.h>


const int numPoints = 300;

const float numTurns = 3.0f;

const float angleStep = 2.0f * 3.1415926f * numTurns / numPoints;

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
    

    // Cria a janela
    GLFWwindow* window = glfwCreateWindow(800, 600, "FraKk's cool Window 2", NULL, NULL);
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

    // Função de callback para redimensionamento da janela
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height) {
            glViewport(0, 0, width, height);
        });

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

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // Verificar erros de compilação
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERRO::SHADER::FRAGMENT::COMPILACAO_FALHOU\n" << infoLog << std::endl;
    }

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERRO::PROGRAMA::LINKAGEM_FALHOU\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    std::vector<float> vertices;

    float startRadius = 0.9f;        // Começa quase na borda da tela
    float endRadius = 0.05f;         // Termina próximo ao centro
    float radiusStep = (startRadius - endRadius) / numPoints;

    for (int i = 0; i < numPoints; i++) {
        float currentAngle = angleStep * i;
        float currentRadius = startRadius - (radiusStep * i);
        
        float x = currentRadius * cos(currentAngle);
        float y = currentRadius * sin(currentAngle);
        
        // Posição
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(0.0f);
        

        float r =  1.0f;
        float g =  0.0f;
        float b =  0.0f;
        
        vertices.push_back(r);
        vertices.push_back(g);
        vertices.push_back(b);
    }

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    // Loop principal
    while (!glfwWindowShouldClose(window)) {
        // Processa entrada
        processInput(window);
        
        // Renderização
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);  // Cor de fundo escura
        glClear(GL_COLOR_BUFFER_BIT);

        // Usar o shader program
        glUseProgram(shaderProgram);
        

        glBindVertexArray(VAO);

        
        
        glDrawArrays(GL_LINE_STRIP, 0, numPoints);
        
        // Troca os buffers e verifica eventos
        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    // Limpa recursos alocados
    glfwTerminate();
    return 0;
}