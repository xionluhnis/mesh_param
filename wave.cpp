#include <igl/writeOBJ.h>
#include <igl/viewer/Viewer.h>

#include <fstream>
#include <iostream>
#include <string>

using namespace Eigen;
using namespace std;

void usage(const string &basename) {
  cout << 
    "Usage: " << basename << " [options] mesh\n"
    "\n"
    "Outputs mesh.obj (volume) amd mesh_surf.obj (surface).\n"
    "\n"
    "Options:\n"
    " -x    dx    mesh width\n"
    " -y    dy    mesh height\n"
    " -z    dz    base altitude\n"
    " -fx   a,b   frequency range on x axis over [0;1] stretched to [0;dx]\n"
    " -fy   a,b   frequency range on y axis over [0;1] stretched to [0;dy]\n"
    " -ax   a,b   amplitude range on x axis\n"
    " -ay   a,b   amplitude range on y axis\n"
    " -sx   nx    number of samples on the x axis\n"
    " -sy   ny    number of samples on the y axis\n"
    " -abs        use absolute version of sinusoids\n"
    " -arclength  use arc-length normalization uv mapping\n"
    " -normuv     normalize UV locations to be within [0;1]^2"
    " -h/--help   show this help message\n"
    "\n"
    "Notes:\n"
    "   - a,b arguments can be passed a single constant value a\n"
    "   - if a value is given for an axis but not the other,\n"
    "     then the same value is used for the other axis\n"
    "   - default: freq=10 amplitude=z/2.0 x=y=z=1 nx=ny=100\n"
    "   - the surface mesh only has the meshing of the top grid\n"
    "   - the frequency ranges expect a mesh on [0;1]^2, dx/dy are used to stretch it"
    "\n"
    "Equation:\n"
    " z = f(x,y) = z0 + 0.5 * A(x/dx,y/dy) * (1 + sin(F(x/dx,y/dy) * x/dx) + sin(F(x,y) * y/dy))\n"
    " with A(x,y) from (ax,ay) and F(x,y) from (fx,fy) multipled by 2pi\n"
    "\n"
    "Example:\n"
    "   ./mesh_wave -h 10.0 -fx 10.0,100.0 -fy 10.0 -ax 0.0,2.0 -ay 2.0\n"
  ;
}

struct range {
  double from, to;
  range(double x = 0.0) : from(x), to(x) {}
  range(double a, double b) : from(a), to(b) {}
  range(const string &str) {
    stringstream ss(str);
    ss >> from;
    if(str.find(",") != string::npos){
      string tmp;
      getline(ss, tmp, ','); // move stream to after the ,
      ss >> to;
    } else {
      to = from;
    }
  }

  bool isNull() const { return from == 0.0 && to == 0.0; }
  double operator()(double x) const {
    return from + x * (to - from);
  }
  double max() const {
    return std::max(from, to);
  }
};

int main(int argc, char *argv[]) {
  
  string filename = "mesh";
  
  double dx = 1.0, dy = 1.0, dz = 1.0;
  double sx = 100.0, sy = 0.0;
  range fx(10), fy;
  range ax, ay;
  bool abs = false, arclength = false, normuv = false;

  for(int i = 1; i < argc; ++i){
    string param = argv[i];
    // no-value parameter
    if(param == "-abs"){
      abs = true;
      continue;

    } else if(param == "-arclength"){
      arclength = true;
      continue;

    } else if(param == "-normuv"){
      normuv = true;
      continue;

    } else if(param == "-h" || param == "--help"){
      usage(argv[0]);
      return 0;
    }
    // name
    if(i == argc - 1){
      if(param[0] == '-'){
        cerr << "Mesh name cannot start with -\n";
        return 1;
      }
      filename = param;
      break;
    }

    // value parameters
    string names[] = { "-x", "-y", "-z", "-fx", "-fy", "-ax", "-ay", "-sx", "-sy" };
    bool   isrng[] = { false, false, false, true, true, true, true, false, false };
    void * ptrs[]  = { &dx, &dy, &dz, &fx, &fy, &ax, &ay, &sx, &sy };
    void * pair[]  = { NULL, NULL, NULL, &fy, &fx, &ay, &ax, NULL, NULL };
    try {
      bool found = false;
      for(size_t j = 0; j < 9; ++j){
        if(param != names[j])
          continue;
        string value = argv[i+1];
        if(isrng[j]){
          // update range parameter value
          range *r = reinterpret_cast<range*>(ptrs[j]);
          *r = range(value);
          // set pair if it is null
          range *r2 = reinterpret_cast<range*>(pair[j]);
          if(r2->isNull())
            *r2 = *r;

        } else {
          double *d = reinterpret_cast<double*>(ptrs[j]);
          stringstream ss(value);
          double val;
          ss >> val;
          *d = val;
        }

        found = true;
        break;
      }

      if(!found){
        // invalid parameter?
        usage(argv[0]);
        cerr << "Unknown parameter: " << param << "\n";
        return 1;
      } else {
        i += 1; // we used one argument for the value
      }

    } catch(string err){
      usage(argv[0]);
      cerr << "Problem when reading parameters: " << err << "\n";
      return 1;
    }
  }

  // sampling
  size_t nx = sx, ny = sy;
  if(ny == 0)
    ny = nx;
  if(nx == 0){
    cerr << "-sx should be non-negative!\n";
    return 2;
  }

  // amplitude
  if(ax.isNull() || ay.isNull()){
    ax = 1;
    ay = 1;
  }

  // debug parameters
  cout << "(x,y,z) = " << dx << "," << dy << "," << dz << "\n";
  cout << "(nx,ny) = " << nx << "," << ny << "\n";
  cout << "fx in [" << fx.from << ";" << fx.to << "], ";
  cout << "fy in [" << fy.from << ";" << fy.to << "]\n";
  cout << "ax in [" << ax.from << ";" << ax.to << "], ";
  cout << "ay in [" << ay.from << ";" << ay.to << "]\n";

  // surface mesh
  size_t N = nx * ny;
  size_t M = (nx - 1) * (ny - 1) * 2;
  MatrixXd Vs(N, 3); // vertices of surface
  MatrixXi Fs(M, 3); // faces of surface
  MatrixXd UVs(N, 2); // uv mapping
  // volume mesh
  MatrixXd Vv(2 * N, 3);
  MatrixXi Fv(
    2 * M // top and bottom faces
      + (nx - 1) * 4 + (ny - 1) * 4, // sides
    3 // triangles
  );

  // set vertices
  size_t j = 0;
  for(size_t i = 0, x = 0, y = 0; i < Vs.rows(); ++i, ++x){
    if(x >= nx){
      x = 0;
      ++y;
    }

    // set vertices (x,y)
    Vs(i, 0) = x * dx / nx;
    Vs(i, 1) = y * dy / ny;

    // volume vertices
    Vv(i, 0) = Vv(i + N, 0) = Vs(i, 0);
    Vv(i, 1) = Vv(i + N, 1) = Vs(i, 1);

    double xp = x / double(nx-1);
    double yp = y / double(ny-1);

    // function z = f(x, y)
    double Axy = ax(xp) + ay(yp);
    double Sx = sin(2.0 * M_PI * fx(xp) * xp);
    double Sy = sin(2.0 * M_PI * fy(yp) * yp);
    if(abs)
      Vs(i, 2) = Axy * std::abs(Sx + Sy);
    else
      Vs(i, 2) = Axy * (Sx + Sy);
    
    // uv map
    if(!arclength){
      // default (x,y) uv map
      UVs(i, 0) = xp;
      UVs(i, 1) = yp;
    } else {
      // arclength-normalized uv map
      // - x / u
      if(x == 0)
        UVs(i, 0) = 0.0;
      else
        UVs(i, 0) = UVs(i - 1, 0) + (Vs.row(i) - Vs.row(i - 1)).norm();
      // - y / v
      if(y == 0)
        UVs(i, 1) = 0.0;
      else
        UVs(i, 1) = UVs(i - nx, 1) + (Vs.row(i) - Vs.row(i - nx)).norm();
    }

    // volume height
    Vv(i, 2) = Vs(i, 2);
    Vv(i + N, 2) = 0.0;

    // set two triangle faces
    if(x + 1 >= nx || y + 1 >= ny)
      continue;
    Fs(j, 0) = i;
    Fs(j, 1) = i + 1;
    Fs(j, 2) = i + nx;
    for(size_t k = 0; k < 3; ++k){
      Fv(j, k)      = Fs(j, k);
      Fv(j + M, k)  = Fs(j, 2 - k) + N; // bottom has inverse orientation
    }
    ++j;
    Fs(j, 0) = i + 1;
    Fs(j, 1) = i + nx + 1;
    Fs(j, 2) = i + nx;
    for(size_t k = 0; k < 3; ++k){
      Fv(j, k)      = Fs(j, k);
      Fv(j + M, k)  = Fs(j, 2 - k) + N; // bottom has inverse orientation
    }
    ++j;
  }
  cout << "Surface faces: " << j << " out of " << Fs.rows() << "\n";
  j *= 2; // we already did all the bottom faces

  // moving surface up using minimum to make sure the base is safe
  Vs.col(2).array() += dz - Vs.col(2).minCoeff();
  Vv.block(0, 2, N, 1) = Vs.col(2);

  // extra faces for volume (sides)
  for(size_t y = 0; y < ny - 1; ++y){
    // x=0 faces
    Fv(j, 0) = y * nx;
    Fv(j, 1) = (y + 1) * nx + N;
    Fv(j, 2) = y * nx + N;
    ++j;
    Fv(j, 0) = y * nx;
    Fv(j, 1) = (y + 1) * nx;
    Fv(j, 2) = (y + 1) * nx + N;
    ++j;
    // x=dx faces
    for(size_t f = 0; f < 2; ++f){
      for(size_t k = 0; k < 3; ++k)
        Fv(j, k) = Fv(j-2, 2-k) + nx - 1;
      ++j;
    }
  }
  for(size_t x = 0; x < nx - 1; ++x){
    // y=0 faces
    Fv(j, 0) = x + N;
    Fv(j, 1) = x + 1;
    Fv(j, 2) = x;
    ++j;
    Fv(j, 0) = x + N;
    Fv(j, 1) = x + N + 1;
    Fv(j, 2) = x + 1;
    ++j;
    // y=dy faces
    for(size_t f = 0; f < 2; ++f){
      for(size_t k = 0; k < 3; ++k)
        Fv(j, k) = Fv(j-2, 2-k) + N - nx;
      ++j;
    }
  }
  cout << "Volume faces: " << j << " out of " << Fv.rows() << "\n";
  
  // normalize uv
  if(normuv){
    for(size_t i = 0; i < 2; ++i){
      // go to [0;inf)^2
      UVs.col(i).array() -= double(UVs.col(i).minCoeff());
      // go to [0;1]^2
      UVs.col(i).array() *= 1.0/ std::max(1e-6, UVs.col(i).maxCoeff());
    }
  }

  // write mesh
  MatrixXd Vn;
  MatrixXi Fn;
  igl::writeOBJ(filename + "_surf.obj", Vs, Fs, Vn, Fn, UVs, Fs);
  igl::writeOBJ(filename + ".obj", Vv, Fv);

  return 0;
}
