//Feito por Frank Vega, Gabriel de Sá e Frederico Prado Chaves

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


// STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace std;

int count = 0;

// Estrutura de caractere
struct Character {
    unsigned int TextureID;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    unsigned int Advance;

    Character() {} // construtor padrão

    Character(GLuint textureID, glm::ivec2 size, glm::ivec2 bearing, unsigned int advance)
    : TextureID(textureID), Size(size), Bearing(bearing), Advance(advance) {}
};

std::map<char, Character> Characters;
unsigned int textVAO, textVBO;
unsigned int textShaderProgram;

// Dimensões da janela
const GLuint WIDTH = 800, HEIGHT = 600;

// Variáveis globais de controle da câmera (posição, direção e orientação)
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 20.0f);
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
    GLuint texID;
};

int selecaoX, selecaoY, selecaoZ;
int TAM;            // Agora TAM será definido dinamicamente
Voxel ***grid = nullptr; // Ponteiro triplo para alocação dinâmica

glm::vec4 colorList[] = {
    {0.5f, 0.5f, 0.5f, 0.5f}, // cinza     0   -- reservado para a interface
    {1.0f, 0.0f, 0.0f, 1.0f}, // vermelho  1
    {0.0f, 1.0f, 0.0f, 1.0f}, // verde     2
    {0.0f, 0.0f, 1.0f, 1.0f}, // azul      3
    {1.0f, 1.0f, 0.0f, 1.0f}, // amarelo   4
    {1.0f, 0.0f, 1.0f, 1.0f}, // magenta   5
    {0.0f, 1.0f, 1.0f, 1.0f}, // ciano     6
    {1.0f, 1.0f, 1.0f, 1.0f}, // branco    7
    {0.0f, 0.0f, 0.0f, 1.0f}, // preto     8  
};

std::string nomesTexturas[] = {
    "Vazio",     // 0
    "Grama",     // 1
    "Vidro",     // 2
    "Esponja",   // 3
    "Pedra",     // 4
    "Terra",     // 5
    "Madeira",   // 6
    "Gelo",      // 7
    "Ouro",      // 8
    "Mel"        // 9
};

// "Paleta" de blocos -- IDs das texturas
GLuint texIDList[10];

int texturaEscolhida = 1;

// Código do Vertex Shader
const GLchar *vertexShaderSource = R"glsl(
 #version 450
 layout (location = 0) in vec3 position;
 layout (location = 1) in vec2 texc;
 
 uniform mat4 view;
 uniform mat4 proj;
 uniform mat4 model;
 out vec2 tex_coord;
 void main()
 {
	tex_coord = vec2(texc.s,1.0-texc.t);
	gl_Position =  proj * view * model * vec4(position, 1.0);
 }
 )glsl";

// Código do Fragment Shader
const GLchar *fragmentShaderSource = R"glsl(
#version 450
in vec2 tex_coord;
out vec4 color;

uniform sampler2D tex_buff;
uniform vec4 uColor;
uniform bool useColor;

void main()
{
    if (useColor)
        color = uColor;
    else
        color = texture(tex_buff, tex_coord);
}
)glsl";

const char* textVertexShader = R"(
#version 450 core
layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
out vec2 TexCoords;
uniform mat4 projection;
void main()
{
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
)";

const char* textFragmentShader = R"(
#version 450 core
in vec2 TexCoords;
out vec4 color;
uniform sampler2D text;
uniform vec3 textColor;
void main()
{
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
    color = vec4(textColor, 1.0) * sampled;
}
)";

unsigned int compileShaderText(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    return shader;
}

unsigned int createTextShaderProgram() {
    unsigned int vs = compileShaderText(GL_VERTEX_SHADER, textVertexShader);
    unsigned int fs = compileShaderText(GL_FRAGMENT_SHADER, textFragmentShader);
    unsigned int program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

void RenderText(GLuint shader, std::string text, float x, float y, float scale, glm::vec3 color)
{
    // Ativa o shader de texto
    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    // Para cada caractere
    for (const char& c : text)
    {
        Character ch = Characters[c];

        // Se o caractere for espaço, apenas avance o cursor
        if (c == ' ') {
            x += (ch.Advance >> 6) * scale;
            continue;
        }

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;

        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += (ch.Advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

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
                        << v.corPos << " " << v.texID << "\n";
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
                arquivo >> v.pos.x >> v.pos.y >> v.pos.z >> v.fatorEscala >> v.visivel >> v.selecionado >> v.corPos >> v.texID;
            }
        }
    }

    arquivo.close();
}

// Cabeçalhos de algumas função
int loadTexture(string filePath); 

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

    // printf("Contador atual: %d\n",count);
    //count++;
    // salvar do arquivo - F1
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
    {
        salvarGradeVoxel("voxel.txt");
    }

    // carregar do arquivo - F2
    if (key == GLFW_KEY_F2 && action == GLFW_PRESS)
    {
        carregarGradeVoxel("voxel.txt");
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

    if ((key == GLFW_KEY_SPACE) && action == GLFW_PRESS)
    {
        grid[selecaoY][selecaoX][selecaoZ].texID = texturaEscolhida;
        grid[selecaoY][selecaoX][selecaoZ].corPos = texturaEscolhida; 
        grid[selecaoY][selecaoX][selecaoZ].visivel = true;

        std::cout << "Bloco atualizado para textura " << texturaEscolhida << std::endl;
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

    // muda a cor do voxel
    bool mudouCor = false;
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        // printf("Entrou no C\n");
        int corAtual = grid[selecaoY][selecaoX][selecaoZ].corPos;
        int texID_atual = grid[selecaoY][selecaoX][selecaoZ].texID;
        if (texID_atual < 5)
        {
            corAtual++;
            printf("Troquei a cor para %d\n", corAtual);
        }
        else
        {
            // printf("Volta pra cor inicial\n");
            corAtual = 0;
            printf("Troquei a cor para %d\n", corAtual);
        }
        texID_atual = (texID_atual + 1) % 3;
        mudouCor = true;
        grid[selecaoY][selecaoX][selecaoZ].corPos = corAtual;
        grid[selecaoY][selecaoX][selecaoZ].texID = texID_atual;
    }

    if (action == GLFW_PRESS) {
    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
        texturaEscolhida = key - GLFW_KEY_1 + 1;
        std::cout << "Textura escolhida: " << texturaEscolhida << std::endl;
    } else if (key == GLFW_KEY_0) {
        texturaEscolhida = 0;
        std::cout << "Textura escolhida: " << texturaEscolhida << std::endl;
    }
}

    // printf("\n\n\n");
}

// Processa as teclas pressionadas para movimentar a câmera no espaço 3D
void processInput(GLFWwindow *window)
{
    float cameraSpeed = 5.0f * deltaTime;
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

   // Coords de texturas arrumadas!
   GLfloat vertices[] = {
    // Layout do vértice:
    // x   y     z    s    t
    // Face da frente (z = +0.5)
     0.5,  0.5,  0.5, 1.0, 1.0,  // topo dir
     0.5, -0.5,  0.5, 1.0, 0.0,  // base dir
    -0.5, -0.5,  0.5, 0.0, 0.0,  // base esq

     0.5,  0.5,  0.5, 1.0, 1.0,  // topo dir
    -0.5, -0.5,  0.5, 0.0, 0.0,  // base esq
    -0.5,  0.5,  0.5, 0.0, 1.0,  // topo esq

    // Face de trás (z = -0.5)
    -0.5,  0.5, -0.5, 1.0, 1.0,  // topo esq
    -0.5, -0.5, -0.5, 1.0, 0.0,  // base esq
     0.5, -0.5, -0.5, 0.0, 0.0,  // base dir

    -0.5,  0.5, -0.5, 1.0, 1.0,  // topo esq
     0.5, -0.5, -0.5, 0.0, 0.0,  // base dir
     0.5,  0.5, -0.5, 0.0, 1.0,  // topo dir

    // Face esquerda (x = -0.5)
    -0.5,  0.5,  0.5, 0.0, 1.0,  // topo frente
    -0.5, -0.5,  0.5, 0.0, 0.0,  // base frente
    -0.5, -0.5, -0.5, 1.0, 0.0,  // base trás

    -0.5,  0.5,  0.5, 0.0, 1.0,  // topo frente
    -0.5, -0.5, -0.5, 1.0, 0.0,  // base trás
    -0.5,  0.5, -0.5, 1.0, 1.0,  // topo trás

    // Face direita (x = +0.5)
     0.5,  0.5, -0.5, 1.0, 1.0,  // topo trás
     0.5, -0.5, -0.5, 1.0, 0.0,  // base trás
     0.5, -0.5,  0.5, 0.0, 0.0,  // base frente

     0.5,  0.5, -0.5, 1.0, 1.0,  // topo trás
     0.5, -0.5,  0.5, 0.0, 0.0,  // base frente
     0.5,  0.5,  0.5, 0.0, 1.0,  // topo frente

    // Face de baixo (y = -0.5)
    -0.5, -0.5,  0.5, 0.0, 1.0,  // frente esq
     0.5, -0.5,  0.5, 1.0, 1.0,  // frente dir
     0.5, -0.5, -0.5, 1.0, 0.0,  // trás dir

    -0.5, -0.5,  0.5, 0.0, 1.0,  // frente esq
     0.5, -0.5, -0.5, 1.0, 0.0,  // trás dir
    -0.5, -0.5, -0.5, 0.0, 0.0,  // trás esq

    // Face de cima (y = +0.5)
    -0.5,  0.5, -0.5, 0.0, 0.0,  // trás esq
     0.5,  0.5, -0.5, 1.0, 0.0,  // trás dir
     0.5,  0.5,  0.5, 1.0, 1.0,  // frente dir

    -0.5,  0.5, -0.5, 0.0, 0.0,  // trás esq
     0.5,  0.5,  0.5, 1.0, 1.0,  // frente dir
    -0.5,  0.5,  0.5, 0.0, 1.0   // frente esq
};

    GLuint VBO, vao;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &VBO);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 1 atributo - coordenadas x, y, z
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

     // 2 atributo - coordenadas de textura s, t 
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return vao;
}

void setColor(GLuint shaderID, glm::vec4 cor)
{
    GLint loc = glGetUniformLocation(shaderID, "uColor");
    glUniform4f(loc, cor.r, cor.g, cor.b, cor.a);
}

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
                grid[y][x][z].texID = 0;
            }
        }
    }
}

bool initFreeType() {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "Erro ao inicializar FreeType\n";
        return false;
    }

    FT_Face face;
    const char* fontPath = "C:/Windows/Fonts/arial.ttf"; // ou forneça outra
    if (FT_New_Face(ft, fontPath, 0, &face)) {
        std::cerr << "Erro ao carregar fonte: " << fontPath << "\n";
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                     face->glyph->bitmap.width,
                     face->glyph->bitmap.rows,
                     0, GL_RED, GL_UNSIGNED_BYTE,
                     face->glyph->bitmap.buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character(
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        );

        Characters[static_cast<char>(c)] = character;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    return true;
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


    if (!initFreeType()) return -1;



    textShaderProgram = createTextShaderProgram();
    glm::mat4 projection = glm::ortho(0.0f, (float)WIDTH, 0.0f, (float)HEIGHT);
    glUseProgram(textShaderProgram);
    glUniform1i(glGetUniformLocation(textShaderProgram, "text"), 0); 
    glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);




    shaderID = setupShader();
    VAO = setupGeometry();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //--------------------------
    texIDList[0] = loadTexture("../assets/block_tex/empty.png");
    texIDList[2] = loadTexture("../assets/block_tex/glass.png");
    texIDList[1] = loadTexture("../assets/block_tex/moss_block.png");
    texIDList[3] = loadTexture("../assets/block_tex/sponge.png");
    texIDList[4] = loadTexture("../assets/block_tex/polished_blackstone_bricks.png");
    texIDList[5] = loadTexture("../assets/block_tex/packed_mud.png");
    texIDList[6] = loadTexture("../assets/block_tex/loom_bottom.png");
    texIDList[7] = loadTexture("../assets/block_tex/frosted_ice_0.png");
    texIDList[8] = loadTexture("../assets/block_tex/gold_block.png");
    texIDList[9] = loadTexture("../assets/block_tex/honey_block_top.png");

    //-------------------------

    float xPos, yPos, zPos;

    selecaoX = 0;
    selecaoY = 0;
    selecaoZ = TAM - 1;

    inicializarGradeVoxel(51);

    //defino o bloco inicialmente selecionado
    selecaoX = 0;
    selecaoY = 0;
    selecaoZ = TAM - 1;

    grid[selecaoY][selecaoX][selecaoZ].selecionado = true;
    


    // Ativando o primeiro buffer de textura do OpenGL
	glActiveTexture(GL_TEXTURE0);

    glUseProgram(shaderID);

    // Define textura no slot 0
    glUniform1i(glGetUniformLocation(shaderID, "tex_buff"), 0);

    // Inicializa useColor como false por padrão
    glUniform1i(glGetUniformLocation(shaderID, "useColor"), GL_FALSE);

    // Cor padrão caso precise
    glUniform4f(glGetUniformLocation(shaderID, "uColor"), 1.0f, 1.0f, 1.0f, 1.0f);

	// Criando a variável uniform pra mandar a textura pro shader
	glUniform1i(glGetUniformLocation(shaderID, "tex_buff"), 0);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

        // navega na grid tridimensional pelos seus índices
        for (int x = 0; x < TAM; x++)
        {
            for (int y = 0; y < TAM; y++)
            {
                for (int z = 0; z < TAM; z++)
                {
                    
                    //else{
                    //    //setColor(shaderID, colorList[grid[y][x][z].corPos]);
                    //    grid[selecaoY][selecaoX][selecaoZ].texID = 0;
                    //}
                    //se for um voxel visivel
                    if (grid[y][x][z].visivel || grid[y][x][z].selecionado)
                    {
                        float fatorEscala = grid[y][x][z].fatorEscala;
                        transformaObjeto(grid[y][x][z].pos.x, grid[y][x][z].pos.y, grid[y][x][z].pos.z, 0.0f, 0.0f, 0.0f, fatorEscala, fatorEscala, fatorEscala);
                        
                        
                        GLuint texID = grid[y][x][z].texID;
                        if (texID >= 0 && texID < 10)
                        {
                            glBindTexture(GL_TEXTURE_2D, texIDList[texID]);
                        }
                        else
                        {
                            std::cerr << "Erro: texID inválido (" << texID << ") em voxel [" << x << "][" << y << "][" << z << "]\n";
                            glBindTexture(GL_TEXTURE_2D, texIDList[0]); // fallback para textura 'empty'
                        }
                        
                        if (texID == 0) //empty 
                            glDisable(GL_DEPTH_TEST);

                        glDrawArrays(GL_TRIANGLES, 0, 36);

                        if (texID == 0) //empty 
                        {
                            glEnable(GL_DEPTH_TEST);
                        }

                    }
                }
            }
        }


        // 1. Renderiza o cubo selecionado normalmente com sua textura original
        Voxel& voxelSel = grid[selecaoY][selecaoX][selecaoZ];
        float fatorEscala = voxelSel.fatorEscala;

        transformaObjeto(voxelSel.pos.x, voxelSel.pos.y, voxelSel.pos.z,
                        0.0f, 0.0f, 0.0f,
                        fatorEscala, fatorEscala, fatorEscala);

        glUniform1i(glGetUniformLocation(shaderID, "useColor"), GL_FALSE); // usar textura
        if (voxelSel.texID >= 0 && voxelSel.texID < 10)
        {
            glBindTexture(GL_TEXTURE_2D, texIDList[voxelSel.texID]);
        }
        else
        {
            std::cerr << "Erro: texID inválido (" << voxelSel.texID << ") no voxel selecionado.\n";
            glBindTexture(GL_TEXTURE_2D, texIDList[0]); // fallback seguro
        }
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // 2. Agora desenha o halo por cima com cor translúcida
        float haloEscala = fatorEscala * 1.05f;

        transformaObjeto(voxelSel.pos.x, voxelSel.pos.y, voxelSel.pos.z,
                        0.0f, 0.0f, 0.0f,
                        haloEscala, haloEscala, haloEscala);

        glBindTexture(GL_TEXTURE_2D, 0); // sem textura
        glUniform1i(glGetUniformLocation(shaderID, "useColor"), GL_TRUE);
        setColor(shaderID, glm::vec4(1.0f, 1.0f, 0.0f, 0.3f)); // amarelo translúcido
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // volta para textura padrão
        glUniform1i(glGetUniformLocation(shaderID, "useColor"), GL_FALSE);

        // Salva estados anteriores
        glUseProgram(0);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);


        glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(WIDTH), 0.0f, static_cast<float>(HEIGHT));
        glUseProgram(textShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // Chamada real
        std::string textoTextura = "Textura: " + nomesTexturas[texturaEscolhida];
        RenderText(textShaderProgram, textoTextura, 25.0f, 60.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));

        // Restaura estado do shader e VAO do cubo
        glUseProgram(shaderID);
        glBindVertexArray(VAO);
        glActiveTexture(GL_TEXTURE0);


        

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}

int loadTexture(string filePath)
{
	GLuint texID;

	// Gera o identificador da textura na memória
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	int width, height, nrChannels;

	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
		if (nrChannels == 3) // jpg, bmp
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else // png
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}