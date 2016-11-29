#include <igl/arap.h>
#include <igl/boundary_loop.h>
#include <igl/harmonic.h>
#include <igl/lscm.h>
#include <igl/map_vertices_to_circle.h>
#include <igl/writeOBJ.h>
#include <igl/read_triangle_mesh.h>
#include <igl/viewer/Viewer.h>

#include <fstream>
#include <iostream>
#include <string>

std::string filename;

Eigen::MatrixXd V;
Eigen::MatrixXi F;
Eigen::MatrixXd V_arap;
Eigen::MatrixXd V_lscm;
Eigen::MatrixXd V_harm;
Eigen::MatrixXd V_uv;

bool show_uv = false;

bool key_down(igl::viewer::Viewer& viewer, unsigned char key, int modifier);

void usage(const std::string &basename);

int main(int argc, char *argv[])
{
  if(argc > 1) {
    filename = argv[1];
  } else {
    usage(argv[0]);
    std::cerr << "Required mesh filename missing!\n";
    return 1;
  }

  // show parameter
  bool show = false;
  if(argc > 2){
    std::string showStr = argv[2];
    show = showStr == "1" || showStr == "true";
  }

  int maxIter = 100;
  if(argc > 3){
    maxIter = atoi(argv[3]);
  }
  std::cout << "Using maxIter=" << maxIter << "\n";

  // Load a mesh in OFF format
  igl::read_triangle_mesh(filename, V, F);

  // Compute the initial solution for ARAP (harmonic parametrization)
  Eigen::VectorXi bnd;
  igl::boundary_loop(F, bnd);
  Eigen::MatrixXd bnd_uv;
  
  std::cout << "Mapping vertices to circle\n";
  igl::map_vertices_to_circle(V, bnd, bnd_uv);

  std::cout << "Computing harmonic mapping\n";
  igl::harmonic(V, F, bnd, bnd_uv, 1, V_harm);

  std::cout << "Computing ARAP mapping\n";
  {
    // Add dynamic regularization to avoid to specify boundary conditions
    igl::ARAPData arap_data;
    arap_data.with_dynamics = true;
    Eigen::VectorXi b  = Eigen::VectorXi::Zero(0);
    Eigen::MatrixXd bc = Eigen::MatrixXd::Zero(0,0);
    
    // Initialize ARAP
    arap_data.max_iter = maxIter;
    // 2 means that we're going to *solve* in 2d
    arap_precomputation(V, F, 2, b, arap_data);

    // Solve arap using the harmonic map as initial guess
    V_arap = V_harm;
    arap_solve(bc, arap_data, V_arap);
  }

  std::cout << "Computing LSCM mapping\n";
  {
    // Fix two points on the boundary
    Eigen::VectorXi b(2,1);
    igl::boundary_loop(F, bnd);
    b(0) = bnd(0);
    b(1) = bnd(round(bnd.size()/2));
    Eigen::MatrixXd bc(2,2);
    bc << 0,0,1,0;

    // LSCM parametrization
    igl::lscm(V, F, b, bc, V_lscm);
  }

  // Information
  std::cout << "V: " << V.rows() << "," << V.cols() << "\n";
  std::cout << "F: " << F.rows() << "," << F.cols() << "\n";
  std::cout << "Vuv: " << V_arap.rows() << "," << V_arap.cols() << "\n";


  if(show){
    // Select ARAP by default
    V_uv = V_arap;

    // Plot the mesh
    igl::viewer::Viewer viewer;
    viewer.data.set_mesh(V, F);
    viewer.data.set_uv(V_uv * 20.0);
    viewer.callback_key_down = &key_down;

    // Disable wireframe
    viewer.core.show_lines = false;

    // Draw checkerboard texture
    viewer.core.show_texture = true;

    // Launch the viewer
    viewer.launch();
  }

  // write meshes
  Eigen::MatrixXd V0(0,0);
  Eigen::MatrixXi F0(0,0);
  igl::writeOBJ(filename + "_arap.obj", V, F, V0, F0, V_arap, F);
  igl::writeOBJ(filename + "_harm.obj", V, F, V0, F0, V_harm, F);
  igl::writeOBJ(filename + "_lscm.obj", V, F, V0, F0, V_lscm, F);

  return 0;
}

void usage(const std::string &basename) {
  std::cout << "Usage: " << basename << " mesh [show]\n";
  std::cout << "   show: whether to display the mesh (default, 1) or not (0)\n";
  std::cout << "\n";
}

bool key_down(igl::viewer::Viewer& viewer, unsigned char key, int modifier) {

  switch(key){
    case '0':
      show_uv = !show_uv;
      break;

    case '1':
      V_uv = V_arap;
      break;
    case '2':
      V_uv = V_harm;
      break;
    case '3':
      V_uv = V_lscm;
      break;

    default:
      break;
  }

  if (show_uv){
    viewer.data.set_mesh(V_uv * 20.0, F);
    viewer.core.align_camera_center(V_uv, F);
  } else {
    viewer.data.set_mesh(V,F);
    viewer.data.set_uv(V_uv * 20.0);
    viewer.core.align_camera_center(V,F);
  }

  viewer.data.compute_normals();

  return false;
}


