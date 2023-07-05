#include <iostream>
#include <cmath>
#include <unistd.h>
#include <vector>
#define PI 3.1415926535

// yeah global variables, deal with it
const int width = 180, height = 50;
const char background = ' ';
const char symbols[6]{'/', '$', '*', '-', ':', '"'};

// print the character buffer on terminal
void draw(char buffer[])
{
    // clear the screen
    // std::cout << "\x1b[H";
    // move cursor to beginning
    std::cout << "\033[1;1H";

    // draw the buffer
    std::string outstring = "";
    for (int i = 0; i < width * height; ++i)
    {
        if (!(i % width))
            outstring += "\n";
        // std::cout << std::endl;
        // std::cout << buffer[i];
        outstring += buffer[i];
    }
    // std::cout << std::endl;
    std::cout << outstring << std::endl;
}

struct vec3f
{
    float x, y, z;
    int f;
    // this is hacky. If defined, f is the index of the face of the cube that
    // this point lies on. It's just a convenience to keep this information
    // together with the point coordinates. At some point I will write a class.
};

struct vec2f
{
    float x, y;
};

vec3f rotatePointOnZ(vec3f point, float angle)
{
    float newX = point.x * cos(angle) - point.y * sin(angle);
    float newY = point.x * sin(angle) + point.y * cos(angle);
    float newZ = point.z;
    return vec3f{newX, newY, newZ};
}

vec3f rotatePointOnY(vec3f point, float angle)
{
    float newX = point.x * cos(angle) + point.z * sin(angle);
    float newY = point.y;
    float newZ = -point.x * sin(angle) + point.z * cos(angle);
    return vec3f{newX, newY, newZ};
}

vec3f rotatePointOnX(vec3f point, float angle)
{
    float newX = point.x;
    float newY = point.y * cos(angle) - point.z * sin(angle);
    float newZ = point.y * sin(angle) + point.z * cos(angle);
    return vec3f{newX, newY, newZ};
}

// Generate the point cloud (w.r.t. world coordinates) for a cube,
// given position and size.
std::vector<vec3f> getCubeModel(vec3f position, int size)
{
    float cubeX = position.x, cubeY = position.y, cubeZ = position.z;

    int halfSize = size / 2;
    float Xmin = cubeX - halfSize;
    float Xmax = cubeX + halfSize;
    float Ymin = cubeY - halfSize;
    float Ymax = cubeY + halfSize;
    float Zmin = cubeZ - halfSize;
    float Zmax = cubeZ + halfSize;
    // float increment = 0.1;
    float x, y, z;

    std::vector<vec3f> points = {};

    // front face
    z = Zmin;
    for (x = Xmin; x <= Xmax + 0.1; x += 0.4)
        for (y = Ymin; y <= Ymax + 0.1; y += 0.4)
            points.push_back(vec3f{x, y, z, 1});

    // top face
    y = Ymax;
    for (x = Xmin; x <= Xmax + 0.1; x += 0.4)
        for (z = Zmin; z <= Zmax + 0.1; z += 0.4)
            points.push_back(vec3f{x, y, z, 1});

    // bottom face
    y = Ymin;
    for (x = Xmin; x <= Xmax + 0.1; x += 0.4)
        for (z = Zmin; z <= Zmax + 0.1; z += 0.4)
            points.push_back(vec3f{x, y, z, 2});

    // back face
    z = Zmax;
    for (x = Xmin; x <= Xmax + 0.1; x += 0.4)
        for (y = Ymin; y <= Ymax + 0.1; y += 0.4)
            points.push_back(vec3f{x, y, z, 3});

    // right face
    x = Xmax;
    for (y = Ymin; y <= Ymax + 0.1; y += 0.4)
        for (z = Zmin; z <= Zmax + 0.1; z += 0.4)
            points.push_back(vec3f{x, y, z, 4});

    // left face
    x = Xmin;
    for (y = Ymin; y <= Ymax + 0.1; y += 0.4)
        for (z = Zmin; z <= Zmax + 0.1; z += 0.4)
            points.push_back(vec3f{x, y, z, 5});

    return points;
}

// Draw a cube applying a 3d rotation factor.
void drawCube(std::vector<vec3f> cube, vec3f position, char buffer[], float zbuffer[], float zoom, vec3f angle)
{
    for (size_t i = 0; i < cube.size(); ++i)
    {
        vec3f point = cube[i]; // point w.r.t. world
        int face = point.f;

        // point coordinates w.r.t. cube center
        point.x -= (position.x);
        point.y -= (position.y);
        point.z -= (position.z);

        // rotate point around cube center in 3d space
        // care: 3d rotation is NOT commutative!
        point = rotatePointOnX(point, angle.x); // rotated point w.r.t. cube center
        point = rotatePointOnY(point, angle.y);
        point = rotatePointOnZ(point, angle.z);

        // rotated point w.r.t. world
        point.x += (position.x);
        point.y += (position.y);
        point.z += (position.z);

        // project point in 2d plane
        float dz = zoom / point.z;
        float x2d = point.x * dz; // x coord in the 2d plane (w.r.t. to eye projection)
        float y2d = point.y * dz; // y coord in the 2d plane

        // this is needed because in most terminals a character has height
        // around double its width.
        x2d *= 2;

        // transform 2d coordinates into terminal coordinates
        int terminal_x = (width / 2) + x2d;
        int terminal_y = height - ((height / 2) + y2d);

        // transform terminal coordinates into array indices
        int index = terminal_x + terminal_y * width;

        char c = symbols[face];

        if (terminal_x >= 0 && terminal_x < width && terminal_y >= 0 && terminal_y < height)
            if (point.z < zbuffer[index])
            {
                zbuffer[index] = point.z;
                buffer[index] = c;
            }
    }
    // draw(buffer);
}

int main()
{
    // clear the screen, hide cursor
    std::cout << "\033[2J";
    std::cout << "\3[?25l";

    // initialize stuff
    char buffer[width * height];
    float zbuffer[width * height];
    vec3f angle = {0.0, 0.0, 0.0};
    float zoom = 95; // distance eye-2dplane

    // cube 1
    int cubeSize = 20;
    vec3f position = {0, 0, 100};
    // we compute cube 1 outside the loop because it is stationary
    std::vector<vec3f> cube1 = getCubeModel(position, cubeSize);

    // cube 2
    int cubeSize2 = 20;
    vec3f position2 = {34, 0, 200};
    vec3f angle2 = {1, 2, 3};
    float cube2_vx = 0.6;

    while (true)
    {
        // reset buffers
        std::fill(buffer, buffer + width * height, background);
        std::fill(zbuffer, zbuffer + width * height, 100000);

        // get point cloud for cube2
        std::vector<vec3f> cube2 = getCubeModel(position2, cubeSize2);

        // draw cubes into buffer
        drawCube(cube1, position, buffer, zbuffer, zoom, angle);
        drawCube(cube2, position2, buffer, zbuffer, zoom, angle2);
        draw(buffer);

        // update angle 1
        angle.x += 0.01;
        angle.y += 0.02;
        angle.z += 0.05;

        // update angle 2
        angle2.x += 0.07;
        angle2.y += 0.02;
        angle2.z += 0.05;

        if (position2.x > (width / 2) || position2.x < (-width / 2))
            cube2_vx *= -1;
        position2.x += cube2_vx;

        usleep(16'666);
    }

    return 0;
}
