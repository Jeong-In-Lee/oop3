////////////////////////////////////////////////////////////////////////////////
//
// File: virtualLego.cpp
//
// Original Author: ??â?? Chang-hyeon Park, 
// Modified by Bong-Soo Sohn and Dong-Jun Kim
// 
// Originally programmed for Virtual LEGO. 
// Modified later to program for Virtual Billiard.
//        
////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cassert>

IDirect3DDevice9* Device = NULL;

// window size
const int Width = 1024;
const int Height = 768;

const float spherePos[6][2] = { {-2.0f, 0} , {0.0f,0} , {2.0f,0}, {-2.3f, 1.0f}, {0.0f, 1.0f}, {2.3f, 1.0f} };
// initialize the color of each ball
const D3DXCOLOR sphereColor[6] = { d3d::YELLOW, d3d::YELLOW, d3d::YELLOW, d3d::YELLOW,d3d::YELLOW,d3d::YELLOW };

// -----------------------------------------------------------------------------
// Transform matrices
// -----------------------------------------------------------------------------
D3DXMATRIX g_mWorld;
D3DXMATRIX g_mView;
D3DXMATRIX g_mProj;

#define M_RADIUS 0.21   // ball radius
#define PI 3.14159265
#define M_HEIGHT 0.01
#define DECREASE_RATE 0.9982

// -----------------------------------------------------------------------------
// CSphere class definition
// -----------------------------------------------------------------------------

class CSphere {
private:
    float               center_x, center_y, center_z;
    float                   m_radius;
    float               m_velocity_x;
    float               m_velocity_z;
    bool ball_exist = true;
    int ball_color; // 0 - ???? 1 - ???? 2 - ????
    float pre_center_x, pre_center_z;

public:
    CSphere(void)
    {
        D3DXMatrixIdentity(&m_mLocal);
        ZeroMemory(&m_mtrl, sizeof(m_mtrl));
        m_radius = 0;
        m_velocity_x = 0;
        m_velocity_z = 0;
        m_pSphereMesh = NULL;
    }
    ~CSphere(void) {}

public:
    bool create(IDirect3DDevice9* pDevice, D3DXCOLOR color = d3d::WHITE)
    {
        if (NULL == pDevice)
            return false;

        m_mtrl.Ambient = color;
        m_mtrl.Diffuse = color;
        m_mtrl.Specular = color;
        m_mtrl.Emissive = d3d::BLACK;
        m_mtrl.Power = 5.0f;

        if (FAILED(D3DXCreateSphere(pDevice, getRadius(), 50, 50, &m_pSphereMesh, NULL)))
            return false;
        return true;
    }

    void destroy(void)
    {
        if (m_pSphereMesh != NULL) {
            m_pSphereMesh->Release();
            m_pSphereMesh = NULL;
            this->ball_exist = false;
        }
    }

    void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice)
            return;
        pDevice->SetTransform(D3DTS_WORLD, &mWorld);
        pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
        pDevice->SetMaterial(&m_mtrl);
        m_pSphereMesh->DrawSubset(0);
    }

    //?????? ?㫉???? Ȯ??
    bool hasIntersected(CSphere& ball)
    {
        D3DXVECTOR3 position_this = this->getCenter();
        D3DXVECTOR3 position_other = ball.getCenter();
        double xDistance = abs((position_this.x - position_other.x) * (position_this.x - position_other.x));
        double zDistance = abs((position_this.z - position_other.z) * (position_this.z - position_other.z));
        double totalDistance = sqrt(xDistance + zDistance);
        if (totalDistance < (this->getRadius() + ball.getRadius()))
        {
            return true;
        }
        return false;
    }

    void hitBy(CSphere& ball)
    {
        if (this->hasIntersected(ball))
        {
            adjustPosition(ball);
            //???? ???? ?ε????°? 
            float dx = ball.getCenter().x - this->getCenter().x;
            float dz = ball.getCenter().z - this->getCenter().z;
            float distance = sqrt(dx * dx + dz * dz);

            float bvx = ball.m_velocity_x; 
            float bvz = ball.m_velocity_z;

            float velocity = sqrt(bvx * bvx + bvz * bvz);
            float dt = velocity / distance;
            ball.setPower(dx * dt, dz * dt);

            if (this->ball_color == 0)
            {
                this->ball_exist = false;
                this->setCenter(-100, -100, -100);
            }
        }
    }

    void ballUpdate(float timeDiff)
    {
        const float TIME_SCALE = 3.3;
        D3DXVECTOR3 cord = this->getCenter();
        double vx = abs(this->getVelocity_X());
        double vz = abs(this->getVelocity_Z());
        this->pre_center_x = cord.x;
        this->pre_center_z = cord.z;

        if (vx > 0.01 || vz > 0.01)
        {
            float tX = cord.x + TIME_SCALE * timeDiff * m_velocity_x;
            float tZ = cord.z + TIME_SCALE * timeDiff * m_velocity_z;
             this->setCenter(tX, cord.y, tZ);
        }
        else { this->setPower(0, 0); }
    }

    double getVelocity_X() { return this->m_velocity_x; }
    double getVelocity_Z() { return this->m_velocity_z; }

    void setPower(double vx, double vz)
    {
        this->m_velocity_x = vx;
        this->m_velocity_z = vz;
    }

    void setCenter(float x, float y, float z)
    {
        D3DXMATRIX m;
        center_x = x;   center_y = y;   center_z = z;
        D3DXMatrixTranslation(&m, x, y, z);
        setLocalTransform(m);
    }

    bool ball_existance() { return this->ball_exist; }
    float getRadius(void)  const { return (float)(M_RADIUS); }
    const D3DXMATRIX& getLocalTransform(void) const { return m_mLocal; }
    void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }
    D3DXVECTOR3 getCenter(void) const
    {
        D3DXVECTOR3 org(center_x, center_y, center_z);
        return org;
    }

    void setColor(const D3DXCOLOR ball_color)
    {
        if (ball_color == d3d::YELLOW)
            this->ball_color = 0;
        else if (ball_color == d3d::RED)
            this->ball_color = 1;
        else
            this->ball_color = 2;
    }
    void adjustPosition(CSphere& ball) {
        D3DXVECTOR3 ball_cord = ball.getCenter();

        this->setCenter((center_x + this->pre_center_x) / 2, center_y, (center_z + this->pre_center_z) / 2);
        ball.setCenter((ball_cord.x + ball.pre_center_x) / 2, ball_cord.y, (ball_cord.z + ball.pre_center_z) / 2);
        if (this->hasIntersected(ball))
        {
            this->setCenter(this->pre_center_x, center_y, this->pre_center_z);
            ball.setCenter(ball.pre_center_x, ball_cord.y, ball.pre_center_z);
        }
    }
    double getPreCenter_x() const {
        return this->pre_center_x;
    }

    double getPreCenter_z() const {
        return this->pre_center_z;
    }

private:
    D3DXMATRIX              m_mLocal;
    D3DMATERIAL9            m_mtrl;
    ID3DXMesh* m_pSphereMesh;

};



// -----------------------------------------------------------------------------
// CWall class definition
// -----------------------------------------------------------------------------

class CWall {

private:

    float               m_x;
    float m_y;
    float               m_z;
    float                   m_width; // ???? ???̴°? ???? ???? ????
    float                   m_depth; // ???? ???̴°? ???? ???? ????
    float               m_height;
    int wall_position;

public:
    CWall(void)
    {
        D3DXMatrixIdentity(&m_mLocal);
        ZeroMemory(&m_mtrl, sizeof(m_mtrl));
        m_width = 0;
        m_depth = 0;
        m_pBoundMesh = NULL;
    }
    ~CWall(void) {}
public:
    bool create(IDirect3DDevice9* pDevice, float ix, float iz, float iwidth, float iheight, float idepth, D3DXCOLOR color = d3d::WHITE)
    {
        if (NULL == pDevice)
            return false;

        m_mtrl.Ambient = color;
        m_mtrl.Diffuse = color;
        m_mtrl.Specular = color;
        m_mtrl.Emissive = d3d::BLACK;
        m_mtrl.Power = 5.0f;

        m_width = iwidth;
        m_depth = idepth;

        if (FAILED(D3DXCreateBox(pDevice, iwidth, iheight, idepth, &m_pBoundMesh, NULL)))
            return false;
        return true;
    }
    void destroy(void)
    {
        if (m_pBoundMesh != NULL) {
            m_pBoundMesh->Release();
            m_pBoundMesh = NULL;
        }
    }
    void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice)
            return;
        pDevice->SetTransform(D3DTS_WORLD, &mWorld);
        pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
        pDevice->SetMaterial(&m_mtrl);
        m_pBoundMesh->DrawSubset(0);
    }
    // ??ģ?? Ȯ???ϰ?
    bool hasIntersected(CSphere& ball)
    {
        if (this->wall_position == 0)
        {
            if (ball.getCenter().z + ball.getRadius() > this->m_z - (this->m_depth / 2))
            {
                return true;
            }
        }
        else if (this->wall_position == 2)
        {
            if (ball.getCenter().x + ball.getRadius() > this->m_x - (this->m_width / 2))
            {
                return true;
            }
        }
        else if (this->wall_position == 3)
        {
            if (ball.getCenter().x - ball.getRadius() < this->m_x + (this->m_width / 2))
            {
                return true;
            }
        }
        return false;
    }

    // ?浹 ???? (?ӵ??? ???? ?ٲ??ֱ?)
    void hitBy(CSphere& ball)
    {

        if (this->hasIntersected(ball))
        {
            this->adjustPosition(ball);
            if (this->wall_position == 0) // ????
            {
                ball.setPower(ball.getVelocity_X(), -ball.getVelocity_Z());
            }
            else if (this->wall_position == 1) //?Ʒ?
            {
                return;
            }
            else if (this->wall_position == 2) //??????
            {
                ball.setPower(-ball.getVelocity_X(), ball.getVelocity_Z());
            }
            else
            {
                ball.setPower(-ball.getVelocity_X(), ball.getVelocity_Z());
            }
        }

    }

    void setPosition(float x, float y, float z)
    {
        D3DXMATRIX m;
        this->m_x = x;
        this->m_y = y;
        this->m_z = z;

        D3DXMatrixTranslation(&m, x, y, z);
        setLocalTransform(m);
    }

    void set_wallPosition(int numbering)
    {
        this->wall_position = numbering;
    }

    float getHeight(void) const { return M_HEIGHT; }

    void adjustPosition(CSphere& ball) {
        ball.setCenter((ball.getCenter().x + ball.getPreCenter_x()) / 2, ball.getCenter().y, (ball.getCenter().z + ball.getPreCenter_z()) / 2);
        if (this->hasIntersected(ball))
        {
            ball.setCenter(ball.getPreCenter_x(), ball.getCenter().y, ball.getPreCenter_z());
        }
    }



private:
    void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }

    D3DXMATRIX              m_mLocal;
    D3DMATERIAL9            m_mtrl;
    ID3DXMesh* m_pBoundMesh;
};

// -----------------------------------------------------------------------------
// CLight class definition
// -----------------------------------------------------------------------------

class CLight {
public:
    CLight(void)
    {
        static DWORD i = 0;
        m_index = i++;
        D3DXMatrixIdentity(&m_mLocal);
        ::ZeroMemory(&m_lit, sizeof(m_lit));
        m_pMesh = NULL;
        m_bound._center = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        m_bound._radius = 0.0f;
    }
    ~CLight(void) {}
public:
    bool create(IDirect3DDevice9* pDevice, const D3DLIGHT9& lit, float radius = 0.1f)
    {
        if (NULL == pDevice)
            return false;
        if (FAILED(D3DXCreateSphere(pDevice, radius, 10, 10, &m_pMesh, NULL)))
            return false;

        m_bound._center = lit.Position;
        m_bound._radius = radius;

        m_lit.Type = lit.Type;
        m_lit.Diffuse = lit.Diffuse;
        m_lit.Specular = lit.Specular;
        m_lit.Ambient = lit.Ambient;
        m_lit.Position = lit.Position;
        m_lit.Direction = lit.Direction;
        m_lit.Range = lit.Range;
        m_lit.Falloff = lit.Falloff;
        m_lit.Attenuation0 = lit.Attenuation0;
        m_lit.Attenuation1 = lit.Attenuation1;
        m_lit.Attenuation2 = lit.Attenuation2;
        m_lit.Theta = lit.Theta;
        m_lit.Phi = lit.Phi;
        return true;
    }
    void destroy(void)
    {
        if (m_pMesh != NULL) {
            m_pMesh->Release();
            m_pMesh = NULL;
        }
    }
    bool setLight(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice)
            return false;

        D3DXVECTOR3 pos(m_bound._center);
        D3DXVec3TransformCoord(&pos, &pos, &m_mLocal);
        D3DXVec3TransformCoord(&pos, &pos, &mWorld);
        m_lit.Position = pos;

        pDevice->SetLight(m_index, &m_lit);
        pDevice->LightEnable(m_index, TRUE);
        return true;
    }

    void draw(IDirect3DDevice9* pDevice)
    {
        if (NULL == pDevice)
            return;
        D3DXMATRIX m;
        D3DXMatrixTranslation(&m, m_lit.Position.x, m_lit.Position.y, m_lit.Position.z);
        pDevice->SetTransform(D3DTS_WORLD, &m);
        pDevice->SetMaterial(&d3d::WHITE_MTRL);
        m_pMesh->DrawSubset(0);
    }

    D3DXVECTOR3 getPosition(void) const { return D3DXVECTOR3(m_lit.Position); }

private:
    DWORD               m_index;
    D3DXMATRIX          m_mLocal;
    D3DLIGHT9           m_lit;
    ID3DXMesh* m_pMesh;
    d3d::BoundingSphere m_bound;
};


// -----------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------
CWall   g_legoPlane;
CWall   g_legowall[3];
CSphere   g_sphere[6];
CSphere   g_target_whiteball;
CSphere red_ball;
CLight   g_light;
int ball_num = 6;
int wall_num = 3;
bool startflag = false;
double g_camera_pos[3] = { 0.0, 5.0, -8.0 };

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------


void destroyAllLegoBlock(void)
{
}

// initialization
bool Setup()
{
    int i;

    D3DXMatrixIdentity(&g_mWorld);
    D3DXMatrixIdentity(&g_mView);
    D3DXMatrixIdentity(&g_mProj);

    // create plane and set the position
    if (false == g_legoPlane.create(Device, -1, -1, 6.6f, 0.03f, 9, d3d::GREEN)) return false;
    g_legoPlane.setPosition(0.0f, -0.0006f / 5, 0.0f);

    // create walls and set the position. note that there are four walls
    if (false == g_legowall[0].create(Device, -1, -1, 6.6f, 0.3f, 0.12f, d3d::DARKRED)) return false; // ???? ???? ???? ???? & ???????? ???? ???? ????
    {
        g_legowall[0].setPosition(0.0f, 0.12f, 4.5f);
        g_legowall[0].set_wallPosition(0);
    }
    if (false == g_legowall[1].create(Device, -1, -1, 0.12f, 0.3f, 9, d3d::DARKRED)) return false; // ???? ???? ???? ??????
    {
        g_legowall[1].setPosition(3.24f, 0.12f, 0.0f);
        g_legowall[1].set_wallPosition(2);
    }
    if (false == g_legowall[2].create(Device, -1, -1, 0.12f, 0.3f, 9, d3d::DARKRED)) return false; // ???? ???? ???? ????
    {
        g_legowall[2].setPosition(-3.24f, 0.12f, 0.0f);
        g_legowall[2].set_wallPosition(3);
    }

    // create four balls and set the position
    for (i = 0; i < ball_num; i++) {
        if (false == g_sphere[i].create(Device, sphereColor[i])) return false;
        g_sphere[i].setCenter(spherePos[i][0], (float)M_RADIUS, spherePos[i][1]);
        g_sphere[i].setPower(0, 0);
        g_sphere[i].setColor(sphereColor[i]);
    }

    // create white mouse ball for set direction
    if (false == g_target_whiteball.create(Device, d3d::WHITE)) return false;
    g_target_whiteball.setCenter(0.0f, 0.12f, -4.5f);
    g_target_whiteball.setColor(d3d::WHITE);

    //create red ball for set direction
    if (false == red_ball.create(Device, d3d::RED)) return false;
    red_ball.setCenter(0.0f, 0.12f, -4.5f + red_ball.getRadius() * 2);
    red_ball.setColor(d3d::RED);

    // light setting 
    D3DLIGHT9 lit;
    ::ZeroMemory(&lit, sizeof(lit));
    lit.Type = D3DLIGHT_POINT;
    lit.Diffuse = d3d::WHITE;
    lit.Specular = d3d::WHITE * 0.9f;
    lit.Ambient = d3d::WHITE * 0.9f;
    lit.Position = D3DXVECTOR3(0.0f, 4.0f, 0.0f);
    lit.Range = 100.0f;
    lit.Attenuation0 = 0.0f;
    lit.Attenuation1 = 0.9f;
    lit.Attenuation2 = 0.0f;
    if (false == g_light.create(Device, lit))
        return false;

    // Position and aim the camera.
    D3DXVECTOR3 pos(0.0f, 8.0f, -8.0f);
    D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 up(0.0f, 2.0f, 0.0f);
    D3DXMatrixLookAtLH(&g_mView, &pos, &target, &up);
    Device->SetTransform(D3DTS_VIEW, &g_mView);

    // Set the projection matrix.
    D3DXMatrixPerspectiveFovLH(&g_mProj, D3DX_PI / 4,
        (float)Width / (float)Height, 1.0f, 100.0f);
    Device->SetTransform(D3DTS_PROJECTION, &g_mProj);

    // Set render states.
    Device->SetRenderState(D3DRS_LIGHTING, TRUE);
    Device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
    Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);

    g_light.setLight(Device, g_mWorld);
    return true;
}

void Cleanup(void)
{
    g_legoPlane.destroy();
    for (int i = 0; i < wall_num; i++) {
        g_legowall[i].destroy();
    }
    destroyAllLegoBlock();
    g_light.destroy();
}


// timeDelta represents the time between the current image frame and the last image frame.
// the distance of moving balls should be "velocity * timeDelta"
bool Display(float timeDelta)
{
    int i = 0;
    int j = 0;


    if (Device)
    {
        if (startflag)
        {
            Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00afafaf, 1.0f, 0);
            Device->BeginScene();

            // check whether any two balls hit together and update the direction of balls
            for (i = 0; i < ball_num; i++) {
                g_sphere[i].hitBy(red_ball);
            }

            // update the position of each ball. during update, check whether each ball hit by walls.
            for (i = 0; i < ball_num; i++) {
                g_sphere[i].ballUpdate(timeDelta);
            }

            for (i = 0; i < wall_num; i++) {
                g_legowall[i].hitBy(red_ball);
            }

            g_target_whiteball.hitBy(red_ball);
            g_target_whiteball.ballUpdate(timeDelta);

            if (red_ball.getCenter().z < -5.0f)
            {
                CSphere temp;
                temp.create(Device, d3d::RED);
                temp.setCenter(g_target_whiteball.getCenter().x, g_target_whiteball.getCenter().y, g_target_whiteball.getCenter().z + red_ball.getRadius() * 2);
                red_ball.destroy();
                red_ball = temp;
                startflag = false;
            }

            red_ball.ballUpdate(timeDelta);

            // draw plane, walls, and spheres
            g_legoPlane.draw(Device, g_mWorld);
            for (i = 0; i < wall_num; i++) {
                g_legowall[i].draw(Device, g_mWorld);
            }
            for (i = 0; i < ball_num; i++) {
                if (g_sphere[i].ball_existance() == false)
                    continue;
                g_sphere[i].draw(Device, g_mWorld);
            }
            g_target_whiteball.draw(Device, g_mWorld);
            red_ball.draw(Device, g_mWorld);
            g_light.draw(Device);

            Device->EndScene();
            Device->Present(0, 0, 0, 0);
            Device->SetTexture(0, NULL);

            return true;
        }
        else // ???? ???????ų? space?? ???? ?ȴ????? ??
        {
            Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00afafaf, 1.0f, 0);
            Device->BeginScene();


            red_ball.setCenter(g_target_whiteball.getCenter().x, g_target_whiteball.getCenter().y, g_target_whiteball.getCenter().z + red_ball.getRadius()*2);
            red_ball.ballUpdate(timeDelta);

            // draw plane, walls, and spheres
            g_legoPlane.draw(Device, g_mWorld);
            for (i = 0; i < wall_num; i++) {
                g_legowall[i].draw(Device, g_mWorld);
            }
            for (i = 0; i < ball_num; i++) {
                if (g_sphere[i].ball_existance() == false)
                    continue;
                g_sphere[i].draw(Device, g_mWorld);
            }
             
            g_target_whiteball.draw(Device, g_mWorld);
            red_ball.draw(Device, g_mWorld);
            g_light.draw(Device);

            Device->EndScene();
            Device->Present(0, 0, 0, 0);
            Device->SetTexture(0, NULL);

            return true;
        }
    }

}

// ???콺 ?????? ?Ƹ???
LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static bool wire = false;
    static bool isReset = true;
    static int old_x = 0;
    static int old_y = 0;
    static enum { WORLD_MOVE, LIGHT_MOVE, BLOCK_MOVE } move = WORLD_MOVE;

    switch (msg) {
    case WM_DESTROY:
    {
        ::PostQuitMessage(0);
        break;
    }
    case WM_KEYDOWN:
    {
        switch (wParam) {
        case VK_ESCAPE:
            ::DestroyWindow(hwnd);
            break;
        case VK_RETURN:
            if (NULL != Device) {
                wire = !wire;
                Device->SetRenderState(D3DRS_FILLMODE,
                    (wire ? D3DFILL_WIREFRAME : D3DFILL_SOLID));
            }
            break;
        case VK_SPACE:
            if (!startflag)
                red_ball.setPower(0, 2);
            startflag = true;

            break;

        }
        break;
    }

    case WM_MOUSEMOVE:
    {
        int new_x = LOWORD(lParam);
        int new_y = HIWORD(lParam);
        float dx;
        float dy;

        if (LOWORD(wParam) & MK_LBUTTON) {

            if (isReset) {
                isReset = false;
            }
            else {
                D3DXVECTOR3 vDist;
                D3DXVECTOR3 vTrans;
                D3DXMATRIX mTrans;
                D3DXMATRIX mX;
                D3DXMATRIX mY;

                switch (move) {
                case WORLD_MOVE:
                    dx = (old_x - new_x) * 0.01f;
                    dy = (old_y - new_y) * 0.01f;
                    D3DXMatrixRotationY(&mX, dx);
                    D3DXMatrixRotationX(&mY, dy);
                    g_mWorld = g_mWorld * mX * mY;

                    break;
                }
            }

            old_x = new_x;
            old_y = new_y;

        }
        else {
            isReset = true;

            if (LOWORD(wParam) & MK_RBUTTON) {
                dx = (old_x - new_x);// * 0.01f;

                D3DXVECTOR3 coord3d = g_target_whiteball.getCenter();
                g_target_whiteball.setCenter(coord3d.x + dx * (-0.007f), coord3d.y, -4.5f);
            }
            old_x = new_x;
            old_y = new_y;

            move = WORLD_MOVE;
        }
        break;
    }
    }

    return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hinstance,
    HINSTANCE prevInstance,
    PSTR cmdLine,
    int showCmd)
{
    srand(static_cast<unsigned int>(time(NULL)));

    if (!d3d::InitD3D(hinstance,
        Width, Height, true, D3DDEVTYPE_HAL, &Device))
    {
        ::MessageBox(0, "InitD3D() - FAILED", 0, 0);
        return 0;
    }

    if (!Setup())
    {
        ::MessageBox(0, "Setup() - FAILED", 0, 0);
        return 0;
    }

    d3d::EnterMsgLoop(Display);

    Cleanup();

    Device->Release();

    return 0;
}