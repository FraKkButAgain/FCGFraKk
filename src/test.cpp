// vou ser completamente transparente
// muitas das funções foram escritas com ai pra me ajudar, eu honestamente n conseguiria escrever codigo assim e estava com tempo limite
// foi mal se ficar meio ruim mas eu tentei meu melhor pra deixar funcionando!

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_set>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <random>

// ---- stb_image ----
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// --------------------- variaveis ---------------------
static float roomYOffset   = -1.0f;   
static float moveSpeed     = 5.0f;    // velocidade do jogador
static float rotSpeedDeg   = 90.0f;   // velocidade de rotação do player

// Dados das balas
static float bulletSpeed   = 18.0f;   
static float bulletLife    = 2.5f; 
static float bulletRadius  = 0.08f;   // raio (renderizado como um cubo, (eu n consegui fazer esfera... depois resolver isso))

// Colisão
static float cellSize      = 1.75f;   // tamanho dos blocos de colisão pra voxelização, tava lagando demais sem 
static glm::vec3 playerBoxMin(-0.5f, 0.0f, -0.5f); // colisão do player (AABB)
static glm::vec3 playerBoxMax( 0.5f, 2.0f,  0.5f); // aparentemente aabb significa Axis-Aligned Bounding Box :D

// --------------- coisas de opengl --------------------
// Ajusta viewport quando a janela muda de tamanho
static void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}

// Magia negra, nem idea como isso funciona
static GLuint compileShader(GLenum type, const char* src) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
        std::string log; log.resize(len > 0 ? len : 1);
        GLsizei got = 0; glGetShaderInfoLog(sh, len, &got, log.data());
        std::cerr << "[Shader] compile error ("
                  << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT")
                  << ")\n" << log.data() << std::endl;
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

static GLuint linkProgram(GLuint vs, GLuint fs) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::string log; log.resize(len > 0 ? len : 1);
        GLsizei got = 0; glGetProgramInfoLog(prog, len, &got, log.data());
        std::cerr << "[Program] link error\n" << log.data() << std::endl;
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

// Cria uma textura 1x1 branca pra ser um default
static GLuint makeWhiteTexture() {
    unsigned char px[4] = {255, 255, 255, 255};
    GLuint t = 0;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    glBindTexture(GL_TEXTURE_2D, 0);
    return t;
}

// Carrega uma textura de disco//  btw os objetos estão em assets!
static GLuint loadTexture2D(const std::string& path, bool flipY=true) {
    stbi_set_flip_vertically_on_load(flipY ? 1 : 0);
    int w=0, h=0, n=0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &n, 0);
    if (!data) { std::cerr << "[TEX] failed: " << path << "\n"; return 0; }
    GLenum ifmt = (n == 1 ? GL_R8 : (n == 3 ? GL_RGB8 : GL_RGBA8));
    GLenum fmt  = (n == 1 ? GL_RED : (n == 3 ? GL_RGB  : GL_RGBA));
    GLuint tex = 0; glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, ifmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return tex;
}

// eu não estava conseguindo fazer os arquivos serem vistos pelo codigo, então essa função ajudou, da pra remover se conseguir fazer funcionar
static std::string resolvePath(const std::string& rel) {
    static const char* bases[] = { "", "../", "../../", "../../../" };
    for (auto base : bases) {
        std::string cand = std::string(base) + rel;
        std::ifstream f(cand);
        if (f.good()) {
            std::cerr << "[PATH] ok: " << cand << "\n";
            return cand;
        }
    }
    std::error_code ec;
    std::cerr << "[PATH] not found: " << rel
              << " | CWD=" << std::filesystem::current_path(ec).string() << "\n";
    return rel;
}

// --------------------- Mesh / OBJ ---------------------
struct Triangle { glm::vec3 a,b,c; };

// Mantém buffers, AABB local e triângulos (para voxelização)
struct Mesh {
    GLuint vao = 0, vbo = 0;
    GLsizei count = 0;
    glm::vec3 localMin{0}, localMax{0};
    std::vector<Triangle> triangles;
};

// Carrega OBJ
static bool loadObj(const std::string& path, Mesh& out) {
    std::ifstream in(path);
    if (!in.is_open()) { std::cerr << "[OBJ] could not open: " << path << "\n"; return false; }

    std::vector<glm::vec3> P;
    std::vector<glm::vec3> N;
    std::vector<glm::vec2> T;
    P.reserve(50000); N.reserve(50000); T.reserve(50000);

    struct FIdx { int v=-1, t=-1, n=-1; };
    std::vector<float> interleaved; interleaved.reserve(200000);

    auto pushVertex = [&](int iv, int it, int inorm, const glm::vec3& fallbackN) {
        glm::vec3 p = (iv >= 0 && iv < (int)P.size()) ? P[iv] : glm::vec3(0);
        glm::vec2 uv = (it >= 0 && it < (int)T.size()) ? T[it] : glm::vec2(0);
        glm::vec3 nn = (inorm >= 0 && inorm < (int)N.size()) ? N[inorm] : fallbackN;
        interleaved.push_back(p.x); interleaved.push_back(p.y); interleaved.push_back(p.z);
        interleaved.push_back(nn.x); interleaved.push_back(nn.y); interleaved.push_back(nn.z);
        interleaved.push_back(uv.x); interleaved.push_back(uv.y);
    };

    auto parseIndex = [&](const std::string& s)->FIdx {
        FIdx r;
        std::stringstream ss(s);
        std::string a,b,c;
        std::getline(ss, a, '/');
        if (!a.empty()) r.v = std::stoi(a) - 1;
        if (std::getline(ss, b, '/')) {
            if (!b.empty()) r.t = std::stoi(b) - 1;
            if (std::getline(ss, c, '/')) {
                if (!c.empty()) r.n = std::stoi(c) - 1;
            }
        }
        return r;
    };

    out.triangles.clear();
    glm::vec3 bbMin( std::numeric_limits<float>::infinity());
    glm::vec3 bbMax(-std::numeric_limits<float>::infinity());

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0]=='#') continue;
        std::stringstream ls(line);
        std::string tag; ls >> tag;
        if (tag == "v") {
            glm::vec3 p; ls >> p.x >> p.y >> p.z;
            P.push_back(p);
            bbMin = glm::min(bbMin, p); bbMax = glm::max(bbMax, p);
        } else if (tag == "vt") {
            glm::vec2 t; ls >> t.x >> t.y; T.push_back(t);
        } else if (tag == "vn") {
            glm::vec3 n; ls >> n.x >> n.y >> n.z; N.push_back(glm::normalize(n));
        } else if (tag == "f") {
            std::vector<FIdx> face;
            std::string vstr;
            while (ls >> vstr) face.push_back(parseIndex(vstr));
            if (face.size() < 3) continue;
            for (size_t i = 1; i + 1 < face.size(); ++i) {
                FIdx f0 = face[0], f1 = face[i], f2 = face[i+1];
                glm::vec3 p0 = P[(f0.v>=0?f0.v:0)], p1 = P[(f1.v>=0?f1.v:0)], p2 = P[(f2.v>=0?f2.v:0)];
                glm::vec3 fN = glm::normalize(glm::cross(p1 - p0, p2 - p0));
                pushVertex(f0.v, f0.t, f0.n, fN);
                pushVertex(f1.v, f1.t, f1.n, fN);
                pushVertex(f2.v, f2.t, f2.n, fN);
                out.triangles.push_back({p0,p1,p2});
            }
        }
    }

    if (interleaved.empty()) { std::cerr << "[OBJ] empty: " << path << "\n"; return false; }

    out.localMin = bbMin;
    out.localMax = bbMax;


    if (out.vbo) glDeleteBuffers(1, &out.vbo);
    if (out.vao) glDeleteVertexArrays(1, &out.vao);
    glGenVertexArrays(1, &out.vao);
    glGenBuffers(1, &out.vbo);
    glBindVertexArray(out.vao);
    glBindBuffer(GL_ARRAY_BUFFER, out.vbo);
    glBufferData(GL_ARRAY_BUFFER, interleaved.size()*sizeof(float), interleaved.data(), GL_STATIC_DRAW);
    GLsizei stride = sizeof(float)*8;
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)*3));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)*6));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    out.count = (GLsizei)(interleaved.size()/8);
    std::cerr << "[OBJ] loaded: " << path << " verts=" << out.count << "\n";
    return true;
}

// Cria um cubo unitário, essa parte funcionou para as balas :D (eu quero fazer um circulo mas n consegui :c)
static void makeUnitCubeMesh(Mesh& out) {
    static const float verts[] = {
        // pos                // nrm           // uv
        // +X
        0.5f,-0.5f,-0.5f,   1,0,0,  0,0,  0.5f, 0.5f,-0.5f,   1,0,0,  1,0,  0.5f, 0.5f, 0.5f,   1,0,0,  1,1,
        0.5f,-0.5f,-0.5f,   1,0,0,  0,0,  0.5f, 0.5f, 0.5f,   1,0,0,  1,1,  0.5f,-0.5f, 0.5f,   1,0,0,  0,1,
        // -X
       -0.5f,-0.5f,-0.5f,  -1,0,0,  0,0, -0.5f, 0.5f, 0.5f,  -1,0,0,  1,1, -0.5f, 0.5f,-0.5f,  -1,0,0,  1,0,
       -0.5f,-0.5f,-0.5f,  -1,0,0,  0,0, -0.5f,-0.5f, 0.5f,  -1,0,0,  0,1, -0.5f, 0.5f, 0.5f,  -1,0,0,  1,1,
        // +Y
       -0.5f, 0.5f,-0.5f,   0,1,0,  0,0,  0.5f, 0.5f, 0.5f,   0,1,0,  1,1,  0.5f, 0.5f,-0.5f,   0,1,0,  1,0,
       -0.5f, 0.5f,-0.5f,   0,1,0,  0,0, -0.5f, 0.5f, 0.5f,   0,1,0,  0,1,  0.5f, 0.5f, 0.5f,   0,1,0,  1,1,
        // -Y
       -0.5f,-0.5f,-0.5f,   0,-1,0, 0,0,  0.5f,-0.5f,-0.5f,   0,-1,0, 1,0,  0.5f,-0.5f, 0.5f,   0,-1,0, 1,1,
       -0.5f,-0.5f,-0.5f,   0,-1,0, 0,0,  0.5f,-0.5f, 0.5f,   0,-1,0, 1,1, -0.5f,-0.5f, 0.5f,   0,-1,0, 0,1,
        // +Z
       -0.5f,-0.5f, 0.5f,   0,0,1,  0,0,  0.5f, 0.5f, 0.5f,   0,0,1,  1,1, -0.5f, 0.5f, 0.5f,   0,0,1,  0,1,
       -0.5f,-0.5f, 0.5f,   0,0,1,  0,0,  0.5f,-0.5f, 0.5f,   0,0,1,  1,0,  0.5f, 0.5f, 0.5f,   0,0,1,  1,1,
        // -Z
       -0.5f,-0.5f,-0.5f,   0,0,-1, 0,0, -0.5f, 0.5f,-0.5f,   0,0,-1, 0,1,  0.5f, 0.5f,-0.5f,   0,0,-1, 1,1,
       -0.5f,-0.5f,-0.5f,   0,0,-1, 0,0,  0.5f, 0.5f,-0.5f,   0,0,-1, 1,1,  0.5f,-0.5f,-0.5f,   0,0,-1, 1,0
    };
    std::vector<float> v( sizeof(verts)/sizeof(float) );
    std::copy(std::begin(verts), std::end(verts), v.begin());
    out.triangles.clear();
    out.localMin = glm::vec3(-0.5f);
    out.localMax = glm::vec3( 0.5f);

    if (out.vbo) glDeleteBuffers(1, &out.vbo);
    if (out.vao) glDeleteVertexArrays(1, &out.vao);
    glGenVertexArrays(1, &out.vao);
    glGenBuffers(1, &out.vbo);
    glBindVertexArray(out.vao);
    glBindBuffer(GL_ARRAY_BUFFER, out.vbo);
    glBufferData(GL_ARRAY_BUFFER, v.size()*sizeof(float), v.data(), GL_STATIC_DRAW);
    GLsizei stride = sizeof(float)*8;
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)*3));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float)*6));
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    out.count = 36;
}

// --------------------- Modelo ---------------------
struct Model {
    Mesh mesh;
    GLuint tex = 0;
    glm::mat4 M{1.0f};
};

// Define posição/rotação/escala do modelo
static void setTransform(Model& m, const glm::vec3& pos,
                         const glm::vec3& eulerDeg, const glm::vec3& scale) {
    glm::mat4 T = glm::translate(glm::mat4(1), pos);
    glm::mat4 R = glm::mat4(1);
    R = glm::rotate(R, glm::radians(eulerDeg.y), glm::vec3(0,1,0)); // yaw
    R = glm::rotate(R, glm::radians(eulerDeg.x), glm::vec3(1,0,0)); // pitch
    R = glm::rotate(R, glm::radians(eulerDeg.z), glm::vec3(0,0,1)); // roll
    glm::mat4 S = glm::scale(glm::mat4(1), scale);
    m.M = T * R * S;
}

// --------------------- Colisão  ---------------------
struct AABB { glm::vec3 min, max; };

// Teste de interseção entre AABBs
static bool aabbOverlap(const AABB& a, const AABB& b){
    return !(a.max.x <= b.min.x || a.min.x >= b.max.x ||
             a.max.y <= b.min.y || a.min.y >= b.max.y ||
             a.max.z <= b.min.z || a.min.z >= b.max.z);
}

// Voxeliza triângulos em caixas da superfície (em espaço local do room)
static void voxelizeTrianglesToBoxes(const std::vector<Triangle>& tris,
                                     std::vector<AABB>& outLocalBoxes,
                                     float cell)
{
    std::unordered_set<int64_t> occ;
    occ.reserve(tris.size()*4);

    auto toIdx = [&](float v)->int { return (int)std::floor(v / cell); };
    auto pack  = [&](int ix,int iy,int iz)->int64_t{
        return ( ( (int64_t)(ix) & 0x1FFFFF)      ) |
               ( ( (int64_t)(iy) & 0x1FFFFF) <<21 ) |
               ( ( (int64_t)(iz) & 0x1FFFFF) <<42 );
    };
    auto has = [&](int ix,int iy,int iz)->bool{
        int64_t k = pack(ix,iy,iz);
        return occ.find(k) != occ.end();
    };
    auto sfix = [](int v)->int { if(v & (1<<20)) v |= ~0x1FFFFF; return v; };

    for(const auto& t : tris){
        glm::vec3 mn = glm::min(t.a, glm::min(t.b, t.c));
        glm::vec3 mx = glm::max(t.a, glm::max(t.b, t.c));
        int ix0 = toIdx(mn.x), iy0 = toIdx(mn.y), iz0 = toIdx(mn.z);
        int ix1 = toIdx(mx.x), iy1 = toIdx(mx.y), iz1 = toIdx(mx.z);
        for(int iz=iz0; iz<=iz1; ++iz)
        for(int iy=iy0; iy<=iy1; ++iy)
        for(int ix=ix0; ix<=ix1; ++ix){
            glm::vec3 cmin(ix*cell, iy*cell, iz*cell);
            glm::vec3 cmax = cmin + glm::vec3(cell);
            if(!(mx.x < cmin.x || mn.x > cmax.x ||
                 mx.y < cmin.y || mn.y > cmax.y ||
                 mx.z < cmin.z || mn.z > cmax.z))
            {
                occ.insert(pack(ix,iy,iz));
            }
        }
    }

    std::vector<int64_t> surface; surface.reserve(occ.size());
    for(int64_t key : occ){
        int ix =  (int)( key        & 0x1FFFFF);
        int iy =  (int)((key >> 21) & 0x1FFFFF);
        int iz =  (int)((key >> 42) & 0x1FFFFF);
        ix = sfix(ix); iy = sfix(iy); iz = sfix(iz);
        bool nxm = has(ix-1,iy,iz);
        bool nxp = has(ix+1,iy,iz);
        bool nym = has(ix,iy-1,iz);
        bool nyp = has(ix,iy+1,iz);
        bool nzm = has(ix,iy,iz-1);
        bool nzp = has(ix,iy,iz+1);
        bool interior = nxm && nxp && nym && nyp && nzm && nzp;
        if(!interior) surface.push_back(key);
    }

    outLocalBoxes.clear();
    outLocalBoxes.reserve(surface.size());
    for(int64_t key : surface){
        int ix =  (int)( key        & 0x1FFFFF);
        int iy =  (int)((key >> 21) & 0x1FFFFF);
        int iz =  (int)((key >> 42) & 0x1FFFFF);
        ix = sfix(ix); iy = sfix(iy); iz = sfix(iz);
        glm::vec3 mn(ix*cell, iy*cell, iz*cell);
        outLocalBoxes.push_back({ mn, mn + glm::vec3(cell) });
    }
    std::cerr << "[VOX] cells=" << occ.size() << " surface=" << outLocalBoxes.size()
              << " cell=" << cell << "\n";
}

// Converte caixas locais do modelo para mundo
static std::vector<AABB> makeWorldBoxesFromRoom(const Model& room,
                                                const std::vector<AABB>& localBoxes)
{
    std::vector<AABB> res;
    res.reserve(localBoxes.size());
    glm::vec3 t = glm::vec3(room.M[3]);
    float sx = glm::length(glm::vec3(room.M[0]));
    float sy = glm::length(glm::vec3(room.M[1]));
    float sz = glm::length(glm::vec3(room.M[2]));
    for (const auto& b : localBoxes) {
        glm::vec3 wmn = glm::vec3(b.min.x*sx, b.min.y*sy, b.min.z*sz) + t;
        glm::vec3 wmx = glm::vec3(b.max.x*sx, b.max.y*sy, b.max.z*sz) + t;
        res.push_back({ glm::min(wmn, wmx), glm::max(wmn, wmx) });
    }
    return res;
}

// AABB do modelo em mundo
static AABB worldAABBFromModel(const Model& m){
    glm::vec3 t = glm::vec3(m.M[3]);
    float sx = glm::length(glm::vec3(m.M[0]));
    float sy = glm::length(glm::vec3(m.M[1]));
    float sz = glm::length(glm::vec3(m.M[2]));
    glm::vec3 mn = m.mesh.localMin, mx = m.mesh.localMax;
    glm::vec3 wmn = glm::vec3(mn.x*sx, mn.y*sy, mn.z*sz) + t;
    glm::vec3 wmx = glm::vec3(mx.x*sx, mx.y*sy, mx.z*sz) + t;
    return { glm::min(wmn, wmx), glm::max(wmn, wmx) };
}

// faz o jogador não colidir com as caixas
static glm::vec3 collideAndSlide(const glm::vec3& localMin, const glm::vec3& localMax,
                                 const glm::vec3& startPos, const glm::vec3& delta,
                                 const std::vector<AABB>& blockers)
{
    glm::vec3 p = startPos;
    glm::vec3 d = delta;
    int axes[3] = {0, 2, 1}; // X, Z, Y

    for (int i=0; i<3; ++i) {
        int ax = axes[i];
        float move = (ax==0?d.x:(ax==1?d.y:d.z));
        if (move == 0.0f) continue;

        glm::vec3 np = p;
        if (ax==0) np.x += move;
        if (ax==1) np.y += move;
        if (ax==2) np.z += move;

        AABB swept{ np + localMin, np + localMax };
        bool blocked = false;
        float bestMove = move;

        for (const auto& b : blockers) {
            bool overlapX = !(swept.max.x <= b.min.x || swept.min.x >= b.max.x);
            bool overlapY = !(swept.max.y <= b.min.y || swept.min.y >= b.max.y);
            bool overlapZ = !(swept.max.z <= b.min.z || swept.min.z >= b.max.z);
            if (!(overlapX && overlapY && overlapZ)) continue;

            if (move > 0) {
                float allow = b.min[ax] - (p[ax] + localMax[ax]);
                if (allow < bestMove) { bestMove = allow; blocked = true; }
            } else {
                float allow = b.max[ax] - (p[ax] + localMin[ax]);
                if (allow > bestMove) { bestMove = allow; blocked = true; }
            }
        }

        if (ax==0) p.x += bestMove;
        if (ax==1) p.y += bestMove;
        if (ax==2) p.z += bestMove;

        if (blocked) {
            if (ax==0) d.x = 0;
            if (ax==1) d.y = 0;
            if (ax==2) d.z = 0;
        }
    }
    return p;
}

// --------------------- Câmera ---------------------
struct Camera {
    glm::vec3 pos{0.0f, 1.2f, 5.0f};
    float yaw = 0.0f;    // graus
    float pitch = 0.0f;  // graus
};

// Lê teclado e move/rotaciona a câmera 
static void updateCameraInput(GLFWwindow* w, Camera& cam, float dt) {
    if (glfwGetKey(w, GLFW_KEY_LEFT)  == GLFW_PRESS) cam.yaw   -= rotSpeedDeg * dt;
    if (glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS) cam.yaw   += rotSpeedDeg * dt;
    if (glfwGetKey(w, GLFW_KEY_UP)    == GLFW_PRESS) cam.pitch += rotSpeedDeg * dt;
    if (glfwGetKey(w, GLFW_KEY_DOWN)  == GLFW_PRESS) cam.pitch -= rotSpeedDeg * dt;
    cam.pitch = std::clamp(cam.pitch, -89.0f, 89.0f);

    glm::vec3 fwdFlat = glm::normalize(glm::vec3(
        std::cos(glm::radians(cam.yaw)), 0.0f, std::sin(glm::radians(cam.yaw))
    ));
    glm::vec3 right = glm::normalize(glm::cross(fwdFlat, glm::vec3(0,1,0)));

    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) cam.pos += fwdFlat * (moveSpeed * dt);
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) cam.pos -= fwdFlat * (moveSpeed * dt);
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) cam.pos -= right   * (moveSpeed * dt);
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) cam.pos += right   * (moveSpeed * dt);
}

// Retorna o vetor “forward” da câmera 
static glm::vec3 cameraForward(const Camera& cam) {
    float cy = std::cos(glm::radians(cam.yaw));
    float sy = std::sin(glm::radians(cam.yaw));
    float cp = std::cos(glm::radians(cam.pitch));
    float sp = std::sin(glm::radians(cam.pitch));
    return glm::normalize(glm::vec3(cy*cp, sp, sy*cp));
}

static glm::mat4 cameraView(const Camera& cam) {
    glm::vec3 dir = cameraForward(cam);
    return glm::lookAt(cam.pos, cam.pos + dir, glm::vec3(0,1,0));
}

// --------------------- Shaders ---------------------
static const char* VS = R"GLSL(
#version 430 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNrm;
layout(location=2) in vec2 aUV;
uniform mat4 uM, uV, uP;
out vec3 vN;
out vec2 vUV;
void main(){
    vec3 N = mat3(transpose(inverse(uM))) * aNrm;
    vN = N;
    vUV = aUV;
    gl_Position = uP * uV * (uM * vec4(aPos,1.0));
}
)GLSL";

static const char* FS = R"GLSL(
#version 430 core
in vec3 vN;
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uTex;
uniform vec3 uTint;
uniform vec3 uLightDir = normalize(vec3(1.0, 1.0, 1.0));
void main(){
    vec3 N = normalize(vN);
    float diff = max(dot(N, normalize(-uLightDir)), 0.1);
    vec3 albedo = texture(uTex, vUV).rgb * uTint;
    FragColor = vec4(albedo * diff, 1.0);
}
)GLSL";


static void drawModelTinted(const Model& model, GLuint prog,
                            const glm::mat4& V, const glm::mat4& P,
                            const glm::vec3& tint) {
    glUseProgram(prog);
    glUniformMatrix4fv(glGetUniformLocation(prog, "uM"), 1, GL_FALSE, glm::value_ptr(model.M));
    glUniformMatrix4fv(glGetUniformLocation(prog, "uV"), 1, GL_FALSE, glm::value_ptr(V));
    glUniformMatrix4fv(glGetUniformLocation(prog, "uP"), 1, GL_FALSE, glm::value_ptr(P));
    glUniform3fv(glGetUniformLocation(prog, "uTint"), 1, glm::value_ptr(tint));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, model.tex);
    glUniform1i(glGetUniformLocation(prog, "uTex"), 0);
    glBindVertexArray(model.mesh.vao);
    glDrawArrays(GL_TRIANGLES, 0, model.mesh.count);
    glBindVertexArray(0);
}

// --------------------- Balas ---------------------
struct Bullet {
    glm::vec3 pos{0};
    glm::vec3 vel{0};
    float life = 0.0f;
    bool alive = false;
};

// --------------------- Main ---------------------
int main(){
    // Inicia GLFW/GL, cria janela e contexto
    if (!glfwInit()) { std::cerr << "Error: glfwInit\n"; return 1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "OBJ + FPS + Bullets + Room Collision", nullptr, nullptr);
    if (!window) { std::cerr << "Error: glfwCreateWindow\n"; glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Error: gladLoadGLLoader\n";
        glfwDestroyWindow(window); glfwTerminate(); return 1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // Compila/Linka shaders
    GLuint vs = compileShader(GL_VERTEX_SHADER, VS);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, FS);
    if (!vs || !fs) { glfwDestroyWindow(window); glfwTerminate(); return 1; }
    GLuint prog = linkProgram(vs, fs);
    glDeleteShader(vs); glDeleteShader(fs);
    if (!prog) { glfwDestroyWindow(window); glfwTerminate(); return 1; }

    // caminhos dos arquivos
    std::string enemyObj = resolvePath("assets/modelos3D/NebulaPlaceholder.obj");
    std::string enemyTex = resolvePath("assets/texturas/test.png");
    std::string roomObj  = resolvePath("assets/modelos3D/Room.obj");
    std::string roomTex  = resolvePath("assets/texturas/RoomTexture.png");

    // Carrega inimigo 
    Model enemy;
    // coisa pra dar load em um cubo caso o modelo n seja encontrado (MESMO ASSIM N FUNCIONA AS VEZES ARGHH)
    if (!loadObj(enemyObj, enemy.mesh)) {
        std::cerr << "[Enemy] load failed -> using cube fallback\n";
        makeUnitCubeMesh(enemy.mesh);
    }
    enemy.tex = loadTexture2D(enemyTex);
    if (!enemy.tex) enemy.tex = makeWhiteTexture();
    setTransform(enemy, glm::vec3(0,0,0), glm::vec3(0,0,0), glm::vec3(1));

    // Carrega sala 
    Model room;
    if (!loadObj(roomObj, room.mesh)) {
        std::cerr << "[Room] load failed -> using cube fallback\n";
        makeUnitCubeMesh(room.mesh);
        setTransform(room, glm::vec3(0,roomYOffset,0), glm::vec3(0), glm::vec3(10.0f, 6.0f, 10.0f));
    } else {
        setTransform(room, glm::vec3(0,roomYOffset,0), glm::vec3(0), glm::vec3(1));
    }
    room.tex = loadTexture2D(roomTex);
    if (!room.tex) room.tex = makeWhiteTexture();

    // simplifica o modelo da sala em caixas (essa parte é pra reduzir o lag)
    std::vector<AABB> roomLocalBoxes;
    voxelizeTrianglesToBoxes(room.mesh.triangles, roomLocalBoxes, cellSize);
    std::vector<AABB> roomWorldBoxes = makeWorldBoxesFromRoom(room, roomLocalBoxes);

    // Câmera
    Camera cam;
    cam.pos = glm::vec3(0.0f, 1.2f, 5.0f);
    cam.yaw = 0.0f;
    cam.pitch = 0.0f;

    // bala
    Mesh bulletMesh; makeUnitCubeMesh(bulletMesh);
    GLuint whiteTex = makeWhiteTexture();

    std::vector<Bullet> bullets; bullets.reserve(128);
    bool spaceWasDown = false;


    // Loop principal
    double last = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = float(now - last); last = now;

        // inputs e colisão
        glm::vec3 oldPos = cam.pos;
        updateCameraInput(window, cam, dt);                 
        glm::vec3 desire = cam.pos - oldPos;                 
        std::vector<AABB> blockers = roomWorldBoxes;        
        blockers.push_back(worldAABBFromModel(enemy));
        cam.pos = collideAndSlide(playerBoxMin, playerBoxMax, oldPos, desire, blockers);

        // codigo do tiro
        // ainda falta algumas coisas pra terminar isso mas a parte principal é que ele ainda falta colisão em geral
        bool spaceNow = (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS);
        if (spaceNow && !spaceWasDown) {
            Bullet b; b.alive = true; b.life = bulletLife;
            glm::vec3 fwd = cameraForward(cam);
            b.pos = cam.pos + fwd * 0.35f; // “muzzle” à frente
            b.vel = fwd * bulletSpeed;
            bullets.push_back(b);
        }
        spaceWasDown = spaceNow;

        // movimenta as balas (acho que daria pra colocar a colisão aqui...)
        for (auto& b : bullets) {
            if (!b.alive) continue;
            b.pos += b.vel * dt;
            b.life -= dt;
            if (b.life <= 0.0f) b.alive = false;
        }
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                                     [](const Bullet& b){ return !b.alive; }),
                      bullets.end());

        // Matrizes de câmera
        int w,h; glfwGetFramebufferSize(window, &w, &h);
        float aspect = (h > 0) ? (float)w / (float)h : 16.0f/9.0f;
        glm::mat4 P = glm::perspective(glm::radians(60.0f), aspect, 0.05f, 1500.0f);
        glm::mat4 V = cameraView(cam);


        glClearColor(0.07f, 0.07f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Desenha sala e inimigo
        drawModelTinted(room,  prog, V, P, glm::vec3(1.0f));
        drawModelTinted(enemy, prog, V, P, glm::vec3(0.9f, 1.0f, 0.9f));

        // Desenha balas
        Model bulletModel;
        bulletModel.mesh = bulletMesh;
        bulletModel.tex  = whiteTex;
        for (const auto& b : bullets) {
            if (!b.alive) continue;
            glm::mat4 M = glm::translate(glm::mat4(1), b.pos) *
                          glm::scale(glm::mat4(1), glm::vec3(bulletRadius*2.0f));
            bulletModel.M = M;
            drawModelTinted(bulletModel, prog, V, P, glm::vec3(1.0f, 0.3f, 0.2f));
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Finaliza
    glDeleteProgram(prog);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
