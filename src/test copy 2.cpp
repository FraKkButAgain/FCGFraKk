#include <iostream>
#include <fstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <map>
#include <string>



using namespace std;

int count = 0;

int corSelecionada = 0;

// Dimensões da janela
const GLuint WIDTH = 800, HEIGHT = 600;

// Variáveis globais de controle da câmera (posição, direção e orientação)
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 100.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f;
float pitch = 0.0f;
bool firstMouse = true;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
float fov = 45.0f;

// Controle de tempo entre frames
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// IDs de shader e VAO
GLuint shaderID, VAO;
GLFWwindow *window;

struct Voxel
{
    glm::vec3 pos;
    float fatorEscala;
    bool visivel = true, selecionado = false;
    int corPos;
};

int selecaoX, selecaoY, selecaoZ;
int TAM;            // Agora TAM será definido dinamicamente
Voxel ***grid = nullptr; // Ponteiro triplo para alocação dinâmica

glm::vec4 colorList[] = {
    {0.5f, 0.5f, 0.5f, 0.5f}, // cinza
    {1.0f, 0.0f, 0.0f, 1.0f}, // vermelho
    {0.0f, 1.0f, 0.0f, 1.0f}, // verde
    {0.0f, 0.0f, 1.0f, 1.0f}, // azul
    {1.0f, 1.0f, 0.0f, 1.0f}, // amarelo
    {1.0f, 0.0f, 1.0f, 1.0f}  // magenta
};

// Código do Vertex Shader
const GLchar *vertexShaderSource = R"glsl(
    #version 450
    layout(location = 0) in vec3 position;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 proj;
    void main() {
        gl_Position = proj * view * model * vec4(position, 1.0);
    }
)glsl";

// Código do Fragment Shader
const GLchar *fragmentShaderSource = R"glsl(
    #version 450
    uniform vec4 uColor;
    out vec4 color;
    void main() {
        color = uColor;
    }
)glsl";

void salvarGradeVoxel(const std::string &nomeArquivo)
{
    std::ofstream arquivo(nomeArquivo);
    if (!arquivo.is_open())
    {
        std::cerr << "Erro ao abrir arquivo para escrita.\n";
        return;
    }

    arquivo << TAM << "\n"; //grava no arquivo o tamanho da matriz tridimensional na primeira linha

    for (int x = 0; x < TAM; ++x)
    {
        for (int y = 0; y < TAM; ++y)
        {
            for (int z = 0; z < TAM; ++z)
            {
                const Voxel &v = grid[x][y][z];
                arquivo << v.pos.x << " " << v.pos.y << " " << v.pos.z << " "
                        << v.fatorEscala << " "
                        << v.visivel << " " << v.selecionado << " "
                        << v.corPos << "\n";
            }
        }
    }

    arquivo.close();
}

void carregarGradeVoxel(const std::string &nomeArquivo)
{
    std::ifstream arquivo(nomeArquivo);
    if (!arquivo.is_open())
    {
        std::cerr << "Erro ao abrir arquivo para leitura.\n";
        return;
    }

    // Lê o tamanho da matriz
    arquivo >> TAM;

    // Aloca dinamicamente a grade
    grid = new Voxel **[TAM];
    for (int x = 0; x < TAM; ++x)
    {
        grid[x] = new Voxel *[TAM];
        for (int y = 0; y < TAM; ++y)
        {
            grid[x][y] = new Voxel[TAM];
        }
    }

    // Lê os dados dos voxels
    for (int x = 0; x < TAM; ++x)
    {
        for (int y = 0; y < TAM; ++y)
        {
            for (int z = 0; z < TAM; ++z)
            {
                Voxel &v = grid[x][y][z];
                arquivo >> v.pos.x >> v.pos.y >> v.pos.z >> v.fatorEscala >> v.visivel >> v.selecionado >> v.corPos;
            }
        }
    }

    arquivo.close();
}

// Atualiza o viewport ao redimensionar a janela
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Callback para movimentação do mouse — controla rotação da câmera
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
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

    float sensitivity = 0.05f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);

    glm::vec3 right = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0, 1.0, 0.0)));
    cameraUp = glm::normalize(glm::cross(right, cameraFront));
}

// Callback de scroll — altera o FOV (zoom)
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    if (fov >= 1.0f && fov <= 120.0f)
        fov -= yoffset;
    if (fov <= 1.0f)
        fov = 1.0f;
    if (fov >= 120.0f)
        fov = 120.0f;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    // salvar do arquivo - F1
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
    {
        salvarGradeVoxel("minecraft.txt");
    }

    // carregar do arquivo - F2
    if (key == GLFW_KEY_F2 && action == GLFW_PRESS)
    {
        carregarGradeVoxel("minecraft.txt");
    }

    // troca a visibilidade de um voxel selecionado
    if (key == GLFW_KEY_DELETE && action == GLFW_PRESS)
    {
        grid[selecaoY][selecaoX][selecaoZ].visivel = false;
    }
    if (key == GLFW_KEY_V && action == GLFW_PRESS)
    {
        grid[selecaoY][selecaoX][selecaoZ].visivel = true;
    }

    // testa a seleção do voxel na grid
    // Troca a cor do antigo voxel selecionado para cinza

    bool mudouSelecao = false;

    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
    {
        if (selecaoX + 1 < TAM)
        {
            grid[selecaoY][selecaoX][selecaoZ].selecionado = false;
            selecaoX++;
            mudouSelecao = true;
            grid[selecaoY][selecaoX][selecaoZ].selecionado = true;
        }
    }
    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
    {
        if (selecaoX - 1 >= 0)
        {
            grid[selecaoY][selecaoX][selecaoZ].selecionado = false;
            selecaoX--;
            mudouSelecao = true;
            grid[selecaoY][selecaoX][selecaoZ].selecionado = true;
        }
    }

    if (key == GLFW_KEY_UP && action == GLFW_PRESS)
    {
        if (selecaoY + 1 < TAM)
        {
            grid[selecaoY][selecaoX][selecaoZ].selecionado = false;
            selecaoY++;
            mudouSelecao = true;
            grid[selecaoY][selecaoX][selecaoZ].selecionado = true;
        }
    }
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
    {
        if (selecaoY - 1 >= 0)
        {
            grid[selecaoY][selecaoX][selecaoZ].selecionado = false;
            selecaoY--;
            mudouSelecao = true;
            grid[selecaoY][selecaoX][selecaoZ].selecionado = true;
        }
    }

    if (key == GLFW_KEY_PAGE_UP && action == GLFW_PRESS)
    {
        if (selecaoZ + 1 < TAM)
        {
            grid[selecaoY][selecaoX][selecaoZ].selecionado = false;
            selecaoZ++;
            mudouSelecao = true;
            grid[selecaoY][selecaoX][selecaoZ].selecionado = true;
        }
    }
    if (key == GLFW_KEY_PAGE_DOWN && action == GLFW_PRESS)
    {
        if (selecaoZ - 1 >= 0)
        {
            grid[selecaoY][selecaoX][selecaoZ].selecionado = false;
            selecaoZ--;
            mudouSelecao = true;
            grid[selecaoY][selecaoX][selecaoZ].selecionado = true;
        }
    }

    if (action == GLFW_PRESS) {
    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_6) {
        corSelecionada = key - GLFW_KEY_1; // 0 para 1, 1 para 2, etc.
        printf("Cor selecionada: %d\n", corSelecionada);
    }
    }

    // muda a cor do voxel
    bool mudouCor = false;
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        grid[selecaoY][selecaoX][selecaoZ].corPos = corSelecionada;
        grid[selecaoY][selecaoX][selecaoZ].visivel = true;
        printf("Voxel pintado com a cor %d\n", corSelecionada);
    }

    // printf("\n\n\n");
}

// Processa as teclas pressionadas para movimentar a câmera no espaço 3D
void processInput(GLFWwindow *window)
{
    float cameraSpeed = 15.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

// Define a matriz de visualização usando a posição e direção da câmera
void especificaVisualizacao()
{
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    GLuint loc = glGetUniformLocation(shaderID, "view");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(view));
}

// Define a matriz de projeção perspectiva com base no FOV
void especificaProjecao()
{
    glm::mat4 proj = glm::perspective(glm::radians(fov), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    GLuint loc = glGetUniformLocation(shaderID, "proj");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(proj));
}

void transformaObjeto(float xpos, float ypos, float zpos, float xrot, float yrot, float zrot, float sx, float sy, float sz)
{
    glm::mat4 transform = glm::mat4(1.0f); // matriz identidade

    // especifica as transformações sobre o objeto - model
    transform = glm::translate(transform, glm::vec3(xpos, ypos, zpos));

    transform = glm::rotate(transform, glm::radians(xrot), glm::vec3(1, 0, 0));
    transform = glm::rotate(transform, glm::radians(yrot), glm::vec3(0, 1, 0));
    transform = glm::rotate(transform, glm::radians(zrot), glm::vec3(0, 0, 1));

    transform = glm::scale(transform, glm::vec3(sx, sy, sz));

    // Envia os dados para o shader
    GLuint loc = glGetUniformLocation(shaderID, "model");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(transform));
}

// Compila shaders e cria o programa de shader
GLuint setupShader()
{
    GLint success;
    GLchar infoLog[512];

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        cout << "Vertex Shader error:\n"
             << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        cout << "Fragment Shader error:\n"
             << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        cout << "Shader Program Linking error:\n"
             << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Cria o VAO com os vértices e cores do cubo 3D
GLuint setupGeometry()
{
    GLfloat vertices[] = {
        // frente
        0.5, 0.5, 0.5, 0.5, -0.5, 0.5, -0.5, -0.5, 0.5,
        -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, -0.5, 0.5,
        // trás
        0.5, 0.5, -0.5, 0.5, -0.5, -0.5, -0.5, -0.5, -0.5,
        -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, -0.5, -0.5,
        // esquerda
        -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, -0.5,
        -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, 0.5,
        // direita
        0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, -0.5,
        0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, 0.5,
        // baixo
        -0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5,
        0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5,
        // cima
        -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5,
        0.5, 0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, 0.5};

    GLuint VBO, vao;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &VBO);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return vao;
}

void setColor(GLuint shaderID, glm::vec4 cor)
{
    GLint loc = glGetUniformLocation(shaderID, "uColor");
    glUniform4f(loc, cor.r, cor.g, cor.b, cor.a);
}

// inicializa a grid
void inicializarGradeVoxel(int tamanho)
{
    TAM = tamanho;

    // Aloca a matriz
    grid = new Voxel **[TAM];
    for (int y = 0; y < TAM; y++)
    {
        grid[y] = new Voxel *[TAM];
        for (int x = 0; x < TAM; x++)
        {
            grid[y][x] = new Voxel[TAM];
        }
    }

    // Preenche os valores padrão
    for (int y = 0, yPos = -TAM / 2; y < TAM; y++, yPos += 1.0f)
    {
        for (int x = 0, xPos = -TAM / 2; x < TAM; x++, xPos += 1.0f)
        {
            for (int z = 0, zPos = -TAM / 2; z < TAM; z++, zPos += 1.0f)
            {
                grid[y][x][z].pos = glm::vec3(xPos, yPos, zPos);
                grid[y][x][z].corPos = 0;
                grid[y][x][z].fatorEscala = 0.98f;
                grid[y][x][z].visivel = false;
                grid[y][x][z].selecionado = false;
            }
        }
    }
    
    
    
}

// Função principal da aplicação
int main()
{
    glfwInit();
    window = glfwCreateWindow(WIDTH, HEIGHT, "Camera Cube", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);


    shaderID = setupShader();

    VAO = setupGeometry();

    


    

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float xPos, yPos, zPos;

    //inicializo a matriz tridimensional de voxels, todos invisíveis
    inicializarGradeVoxel(101);

    //defino o bloco inicialmente selecionado
    selecaoX = 0;
    selecaoY = 0;
    selecaoZ = TAM - 1;

    grid[selecaoY][selecaoX][selecaoZ].selecionado = true;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderID);

        especificaVisualizacao();
        especificaProjecao();

        // renderizar os objetos
        glBindVertexArray(VAO);

        //Desenha um cubão em volta
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        setColor(shaderID, glm::vec4(1.0f, 1.0f, 1.0f, 0.1f)); // branco ou a cor desejada
        float offset = TAM / 2.0f - 0.5f;                      // Para centralizar corretamente
        transformaObjeto(0.0f, 0.0f, 0.0f,                     // posição central
                         0.0f, 0.0f, 0.0f,                     // rotação
                         TAM, TAM, TAM);                       // escala para envolver a grade
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

                // Monta a string com a cor selecionada
        std::string corStr;
        switch(corSelecionada) {
            case 0: corStr = "cinza"; break;
            case 1: corStr = "vermelho"; break;
            case 2: corStr = "verde"; break;
            case 3: corStr = "azul"; break;
            case 4: corStr = "amarelo"; break;
            case 5: corStr = "magenta"; break;
            default: corStr = "desconhecida";
        }
       
        // navega na grid tridimensional pelos seus índices
        for (int x = 0; x < TAM; x++) {
            for (int y = 0; y < TAM; y++) {
                for (int z = 0; z < TAM; z++) {
                    if (grid[y][x][z].visivel || grid[y][x][z].selecionado) {
                        float f = grid[y][x][z].fatorEscala;
                        // --- desenha o cubo normal ---
                        glm::vec4 baseColor = colorList[grid[y][x][z].corPos];
                        if (grid[y][x][z].selecionado)
                            baseColor += glm::vec4(0.3f);  // seu destaque atual
                        setColor(shaderID, baseColor);
                        transformaObjeto(
                        grid[y][x][z].pos.x, grid[y][x][z].pos.y, grid[y][x][z].pos.z,
                        0.0f, 0.0f, 0.0f,
                        f, f, f
                        );
                        glDrawArrays(GL_TRIANGLES, 0, 36);

                        // --- se for selecionado, desenha o cubo transparente maior ---
                        if (grid[y][x][z].selecionado) {
                            // Não impede o depth do cubo sólido
                            glDepthMask(GL_FALSE);

                            // Cor branca semiarquada
                            setColor(shaderID, glm::vec4(1.0f, 1.0f, 1.0f, 0.3f));

                            // Escala 10% maior
                            float haloScale = f * 1.1f;
                            transformaObjeto(
                            grid[y][x][z].pos.x, grid[y][x][z].pos.y, grid[y][x][z].pos.z,
                            0.0f, 0.0f, 0.0f,
                            haloScale, haloScale, haloScale
                            );

                            glDrawArrays(GL_TRIANGLES, 0, 36);

                            // Restaura depth‐mask
                            glDepthMask(GL_TRUE);
                        }
                    }
                }
            }
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}