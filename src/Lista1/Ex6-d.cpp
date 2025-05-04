#include <iostream>
#include <string>
#include <assert.h>


using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar *vertexShaderSource = R"(
    #version 400
    layout (location = 0) in vec3 position;
    void main()
    {
        gl_Position = vec4(position.x, position.y, position.z, 1.0);
        
    }
    )";
   
// Código fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar *fragmentShaderSource = R"(
    #version 400
    uniform vec4 inputColor;
    out vec4 color;
    void main()
    {
        color = inputColor;
    }
)";

// Adicionando um shader específico para pontos circulares
const GLchar *pointVertexShaderSource = R"(
    #version 400
    layout (location = 0) in vec3 position;
    void main()
    {
        gl_Position = vec4(position.x, position.y, position.z, 1.0);
        gl_PointSize = 20.0;
    }
)";

const GLchar *pointFragmentShaderSource = R"(
    #version 400
    uniform vec4 inputColor;
    out vec4 color;
    void main()
    {
        // Calcular distância do centro do fragmento
        vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
        float dist = dot(circCoord, circCoord);
        
        // Descartar fragmentos fora do círculo
        if (dist > 1.0) {
            discard;
        }
        
        color = inputColor;
    }
)";

void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
        glfwSetWindowTitle(window, "whatever");
}

// Função auxiliar para compilar e linkar shaders
GLuint createShaderProgram(const GLchar* vertexSource, const GLchar* fragmentSource) {
    int success;
    char infoLog[512];
    
    // Compilar vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Compilar fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Linkar shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    // Limpar shaders após linkagem
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Configurar MSAA
    glfwWindowHint(GLFW_SAMPLES, 8);

    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    } 

    // Habilitar recursos
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_PROGRAM_POINT_SIZE); // Importante para gl_PointSize funcionar no shader

    float vertices[] = {
        // first triangle
        0.2, 0.2, 0.0,
        0.6, 0.2, 0.0,
        0.4, 0.6, 0.0,
        // T1
        -0.8, -0.3, 0.0,
        -0.4, -0.1, 0.0,
        -0.7, 0.1, 0.0,
    }; 

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glViewport(0, 0, 800, 600);

    // Criar programas de shader
    GLuint mainShaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);
    GLuint pointShaderProgram = createShaderProgram(pointVertexShaderSource, pointFragmentShaderSource);

    while(!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO);

        // Desenhar triângulos preenchidos
        glUseProgram(mainShaderProgram);
        GLint colorLocation = glGetUniformLocation(mainShaderProgram, "inputColor");
        
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glUniform4f(colorLocation, 1.0f, 0.0f, 0.0f, 1.0f); // vermelho
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Desenhar contornos
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(5.0);
        glUniform4f(colorLocation, 0.0f, 0.0f, 0.0f, 1.0f); // preto
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Desenhar pontos circulares usando shader específico
        glUseProgram(pointShaderProgram);
        colorLocation = glGetUniformLocation(pointShaderProgram, "inputColor");
        glUniform4f(colorLocation, 1.0f, 1.0f, 1.0f, 1.0f); // branco
        glDrawArrays(GL_POINTS, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();   
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(mainShaderProgram);
    glDeleteProgram(pointShaderProgram);

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}