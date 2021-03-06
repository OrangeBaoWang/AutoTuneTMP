#include "compute_factor.hpp"
#include "compute_ilist.hpp"
#include "geometry.hpp"
#include "kernel_reference.hpp"
#include "kernels/calculate_stencil.hpp"
#include "kernels/m2m_interactions.hpp"
#include "taylor.hpp" //for multipole
#include "types.hpp"

#include <chrono>

using namespace octotiger;
using namespace octotiger::fmm;

// space_vector one_space_vector;
// multipole one_multipole;

multipole multipole_value_generator() {
  static double counter = 0.01;
  multipole tmp;
  for (size_t i = 0; i < tmp.size(); i++) {
    tmp[i] = counter;
    counter += 0.01;
  }
  return tmp;
}

space_vector space_vector_value_generator() {
  static double counter = 0.01;
  space_vector tmp;
  for (size_t i = 0; i < NDIM; i++) {
    tmp[i] = counter;
  }
  tmp[3] = 0.0;
  counter += 0.01;
  return tmp;
}

struct input_data {
  std::vector<multipole> M_ptr;
  std::vector<std::shared_ptr<std::vector<space_vector>>> com_ptr;
  std::vector<neighbor_gravity_type> all_neighbor_interaction_data;

  input_data()
      : M_ptr(PATCH_SIZE),
        all_neighbor_interaction_data(geo::direction::count()) {
    std::generate(M_ptr.begin(), M_ptr.end(), multipole_value_generator);

    std::shared_ptr<std::vector<space_vector>> com0 =
        std::shared_ptr<std::vector<space_vector>>(
            new std::vector<space_vector>(PATCH_SIZE));
    std::generate(com0->begin(), com0->end(), space_vector_value_generator);
    com_ptr.push_back(com0);
    for (geo::direction &dir : geo::direction::full_set()) {
      neighbor_gravity_type neighbor_gravity_data;
      gravity_boundary_type neighbor_boundary_data;

      // multipole data
      std::shared_ptr<std::vector<multipole>> M =
          std::shared_ptr<std::vector<multipole>>(
              new std::vector<multipole>(PATCH_SIZE));
      std::generate(M->begin(), M->end(), multipole_value_generator);
      // monopole data, not used
      std::shared_ptr<std::vector<real>> m =
          std::shared_ptr<std::vector<real>>(new std::vector<real>());

      std::shared_ptr<std::vector<space_vector>> x =
          std::shared_ptr<std::vector<space_vector>>(
              new std::vector<space_vector>(PATCH_SIZE));
      std::generate(x->begin(), x->end(), space_vector_value_generator);

      neighbor_boundary_data.M = M;
      neighbor_boundary_data.m = m;
      neighbor_boundary_data.x = x;

      // center of masses data

      neighbor_gravity_data.data = neighbor_boundary_data;
      neighbor_gravity_data.is_monopole = false;
      neighbor_gravity_data.direction = dir;

      all_neighbor_interaction_data[dir] = neighbor_gravity_data;
    }
  }
};

int main(void) {
  std::cout << "in main" << std::endl;

  std::vector<input_data> all_input_data(20);

  for (size_t input_index = 0; input_index < all_input_data.size();
       input_index++) {
    for (size_t repetitions = 0; repetitions < 1; repetitions++) {

      std::vector<multipole> &M_ptr = all_input_data[input_index].M_ptr;
      std::vector<std::shared_ptr<std::vector<space_vector>>> &com_ptr =
          all_input_data[input_index].com_ptr;
      std::vector<neighbor_gravity_type> &all_neighbor_interaction_data =
          all_input_data[input_index].all_neighbor_interaction_data;

      gsolve_type type = gsolve_type::RHO;

      auto start_total = std::chrono::high_resolution_clock::now();

      octotiger::fmm::m2m_interactions interactor(
          M_ptr, com_ptr, all_neighbor_interaction_data, type);

      interactor.compute_interactions(); // includes boundary

      auto end_total = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double, std::milli> duration =
          end_total - start_total;
      std::cout << "new interaction kernel  (total w/o old non-multipole "
                   "boundary, ms): "
                << duration.count() << std::endl;

      // auto start_total = std::chrono::high_resolution_clock::now();

      // std::vector<multipole> L(PATCH_SIZE);
      // std::vector<space_vector> L_c(PATCH_SIZE);

      // // octotiger::fmm::m2m_interactions interactor(
      // //     M_ptr, com_ptr, all_neighbor_interaction_data, type);
      // kernel_reference::compute_interactions_reference(
      //     M_ptr, com_ptr, type, all_neighbor_interaction_data, L, L_c);

      // auto end_total = std::chrono::high_resolution_clock::now();
      // std::chrono::duration<double, std::milli> duration =
      //     end_total - start_total;
      // std::cout << "old interaction kernel  (total w/o old non-multipole "
      //              "boundary, ms): "
      //           << duration.count() << std::endl;

      // interactor.print_local_expansions();
      // interactor.print_center_of_masses();

      // interactor.print_potential_expansions();
      // interactor.print_angular_corrections();
    }
  }

  return 0;
}
