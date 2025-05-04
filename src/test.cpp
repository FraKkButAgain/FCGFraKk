#include <iostream>
#include <string>
#include <assert.h>
#include <vector>

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

// Função para criar um círculo
std::vector<float> createCircle(float cx, float cy, float r, int num_segments, float r_color, float g_color, float b_color) {
    std::vector<float> vertices;
    
    // Ponto central (com cor)
    vertices.push_back(cx);
    vertices.push_back(cy);
    vertices.push_back(0.0f);
    vertices.push_back(r_color);
    vertices.push_back(g_color);
    vertices.push_back(b_color);
    
    for (int i = 0; i <= num_segments; i++) {
        float theta = 2.0f * 3.1415926f * float(i) / float(num_segments);
        float x = r * cosf(theta);
        float y = r * sinf(theta);
        
        // Coordenadas do vértice
        vertices.push_back(cx + x);
        vertices.push_back(cy + y);
        vertices.push_back(0.0f);
        
        // Cor do vértice
        vertices.push_back(r_color);
        vertices.push_back(g_color);
        vertices.push_back(b_color);
    }
    
    return vertices;
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

    // Círculos pretos 
    std::vector<float> circle1 = createCircle(-0.55f, -0.55f, 0.15f, 36, 0.0f, 0.0f, 0.0f); // Círculo preto esquerdo
    std::vector<float> circle2 = createCircle(0.55f, -0.55f, 0.15f, 36, 0.0f, 0.0f, 0.0f);  // Círculo preto direito

    // VAO e VBO para os círculos
    unsigned int circleVAO[2], circleVBO[2];
    glGenVertexArrays(2, circleVAO);
    glGenBuffers(2, circleVBO);

    // Configurar o primeiro círculo
    glBindVertexArray(circleVAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, circleVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, circle1.size() * sizeof(float), &circle1[0], GL_STATIC_DRAW);
    // Posição
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Cor
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Configurar o segundo círculo
    glBindVertexArray(circleVAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, circleVBO[1]);
    glBufferData(GL_ARRAY_BUFFER, circle2.size() * sizeof(float), &circle2[0], GL_STATIC_DRAW);
    // Posição
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Cor
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Dados do carro com cores (posição xyz + cor rgb)
    GLfloat carVertices[] = {
        // Posições XYZ        // Cores RGB (vermelho)
        -0.95f, -0.45f, 0.0f,  1.0f, 0.0f, 0.0f,
        -0.95f,  0.05f, 0.0f,  1.0f, 0.0f, 0.0f,
        -0.65f,  0.05f, 0.0f,  1.0f, 0.0f, 0.0f,
        -0.55f,  0.35f, 0.0f,  1.0f, 0.0f, 0.0f,
         0.25f,  0.35f, 0.0f,  1.0f, 0.0f, 0.0f,
         0.45f,  0.05f, 0.0f,  1.0f, 0.0f, 0.0f,
         0.85f,  0.05f, 0.0f,  1.0f, 0.0f, 0.0f,
         0.85f,  0.05f, 0.0f,  1.0f, 0.0f, 0.0f,
         0.95f,  0.05f, 0.0f,  1.0f, 0.0f, 0.0f,
         0.95f, -0.45f, 0.0f,  1.0f, 0.0f, 0.0f,
         0.70f, -0.45f, 0.0f,  1.0f, 0.0f, 0.0f,
         0.60f, -0.35f, 0.0f,  1.0f, 0.0f, 0.0f,
         0.50f, -0.35f, 0.0f,  1.0f, 0.0f, 0.0f,
         0.40f, -0.45f, 0.0f,  1.0f, 0.0f, 0.0f,
        -0.40f, -0.45f, 0.0f,  1.0f, 0.0f, 0.0f,
        -0.50f, -0.35f, 0.0f,  1.0f, 0.0f, 0.0f,
        -0.60f, -0.35f, 0.0f,  1.0f, 0.0f, 0.0f,
        -0.70f, -0.45f, 0.0f,  1.0f, 0.0f, 0.0f,
        -0.95f, -0.45f, 0.0f,  1.0f, 0.0f, 0.0f
    };

    // Índices para triangulação da figura do carro (usado para preencher a forma)
    GLuint carIndices[] = {
        0, 1, 17,
        1, 2, 17,
        2, 16, 17,
        2, 3, 16,
        3, 15, 16,
        3, 4, 15,
        4, 14, 15,
        4, 5, 14,
        5, 13, 14,
        5, 6, 13,
        6, 12, 13,
        6, 7, 12,
        7, 11, 12,
        7, 8, 11,
        8, 10, 11,
        8, 9, 10
    };

    unsigned int carVAO, carVBO, carEBO;
    glGenVertexArrays(1, &carVAO);
    glGenBuffers(1, &carVBO);
    glGenBuffers(1, &carEBO);

    glBindVertexArray(carVAO);

    glBindBuffer(GL_ARRAY_BUFFER, carVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(carVertices), carVertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, carEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(carIndices), carIndices, GL_STATIC_DRAW);

    // Posição
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Cor
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Loop principal
    while (!glfwWindowShouldClose(window)) {
        // Processa entrada
        processInput(window);

        // Renderização
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);  // Cor de fundo
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Desenhar círculos primeiro (atrás do carro)
        glBindVertexArray(circleVAO[0]);
        glDrawArrays(GL_TRIANGLE_FAN, 0, circle1.size() / 6);
        
        glBindVertexArray(circleVAO[1]);
        glDrawArrays(GL_TRIANGLE_FAN, 0, circle2.size() / 6);

        // Desenhar o carro preenchido
        glBindVertexArray(carVAO);
        glDrawElements(GL_TRIANGLES, sizeof(carIndices)/sizeof(GLuint), GL_UNSIGNED_INT, 0);

        // Troca os buffers e verifica eventos
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Limpa recursos alocados
    glDeleteVertexArrays(1, &carVAO);
    glDeleteBuffers(1, &carVBO);
    glDeleteBuffers(1, &carEBO);
    glDeleteVertexArrays(2, circleVAO);
    glDeleteBuffers(2, circleVBO);
    
    glfwTerminate();
    return 0;
}