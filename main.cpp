#include <igl/arap.h>
#include <igl/boundary_loop.h>
#include <igl/harmonic.h>
#include <igl/map_vertices_to_circle.h>
#include <igl/read_triangle_mesh.h>
#include <igl/viewer/Viewer.h>

#include <fstream>
#include <iostream>
#include <string>

std::string filename;

Eigen::MatrixXd V;
Eigen::MatrixXi F;
Eigen::MatrixXd V_uv;
Eigen::MatrixXd initial_guess;

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

  // Load a mesh in OFF format
  igl::read_triangle_mesh(filename, V, F);

  // Compute the initial solution for ARAP (harmonic parametrization)
  Eigen::VectorXi bnd;

  // Information
  std::cout << "V: " << V.rows() << "," << V.cols() << "\n";
  std::cout << "F: " << F.rows() << "," << F.cols() << "\n";
  std::cout << "N: " << N.rows() << "," << N.cols() << "\n";
  std::cout << "UV: " << UV.rows() << "," << UV.cols() << "\n";
  std::cout << "Fuv: " << Fuv.rows() << "," << Fuv.cols() << "\n";
  std::cout << "Fn: " << Fn.rows() << "," << Fn.cols() << "\n";

}

void usage(const std::string &basename) {
  std::cout << "Usage: " << basename << " mesh [mean | gauss | k1 | k2 | k | all] [show]\n";
  std::cout << "   mean:   mean curvature\n";
  std::cout << "   gauss:  gaussian curvature\n";
  std::cout << "   k1:     first main curvature component\n";
  std::cout << "   k2:     second main curvature component\n";
  std::cout << "   k:      both main curvature components\n";
  std::cout << "   all:    all variants\n";
  std::cout << "\n";
  std::cout << "   show: whether to display the mesh curvature (default, 1) or not (0)\n";
  std::cout << "\n";
}

bool key_down(igl::viewer::Viewer& viewer, unsigned char key, int modifier) {

  switch(key){
    case '1':
      show_uv = false;
      break;
    case '2':
      show_uv = true;
      break;

    case 'q':
      V_uv = initial_guess;
      break;

    default:
      break;
  }

  if (show_uv){
    viewer.data.set_mesh(V_uv,F);
    viewer.core.align_camera_center(V_uv,F);
  } else {
    viewer.data.set_mesh(V,F);
    viewer.core.align_camera_center(V,F);
  }

  viewer.data.compute_normals();

  return false;
}


