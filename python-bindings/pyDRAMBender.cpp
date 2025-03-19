#include <immintrin.h>

#include "dct/api/platform/platform.h"
#include "dct/api/platform/DDR4.h"
#include "dct/program_generator.h"

#define NB_DEBUG

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/array.h>
#include <nanobind/stl/vector.h>
#include <nanobind/ndarray.h>


namespace nb = nanobind;
using namespace DRAMBender;
using namespace DCT;

void foo(int i, int j, const std::array<uint32_t, 16>& v) {
  printf("%d %d %d\n", i, j, v[0]);
};

NB_MODULE(pyDRAMBender, m) {

  m.def("foo", &foo);

  nb::class_<Program>(m, "Program")
    .def(nb::init<>())
    .def("pretty_print", &Program::pretty_print)
    .def("dump_registers", &Program::dump_registers);

  nb::class_<DRAMBender::IPlatform>(m, "Platform")
    .def("execute", nb::overload_cast<Program&>(&DRAMBender::IPlatform::execute))
    .def("execute", nb::overload_cast<std::vector<Program>&>(&DRAMBender::IPlatform::execute))
    .def("close", &DRAMBender::IPlatform::close)
    .def("setAREF", &DRAMBender::IPlatform::setAREF)
    .def("readRegisterDump", &DRAMBender::IPlatform::readRegisterDump)
    .def("resetFPGA", &DRAMBender::IPlatform::resetFPGA)
    .def("fullReset", &DRAMBender::IPlatform::fullReset);

  nb::class_<DDR4, DRAMBender::IPlatform>(m, "DDR4")
    .def(nb::init<int, const std::string&>());

  m.def("get_bitflips",
        [](DRAMBender::IPlatform* platform, const std::vector<WritePattern_t>& data_patterns) {
          const int row_size = 8 * 1024;  // 8KB rows
          int bytes_to_receive = data_patterns.size() * row_size;
          void* row_buffers = _mm_malloc(bytes_to_receive, 32);

          platform->receiveData((void*) row_buffers, bytes_to_receive);
          
          for (size_t i = 0; i < data_patterns.size(); i++) {
            uint64_t* row_buffer_p64 = (uint64_t*) (row_buffers + i * row_size);
            uint64_t* cacheline_p64 = (uint64_t*) data_patterns[i].data();
            for(int cl = 0 ; cl < 128 ; cl++) {
              for (int word = 0 ; word < 8 ; word++) {
                int word_id = cl * 8 + word;
                uint64_t word_read = row_buffer_p64[word_id];
                uint64_t flips_mask = cacheline_p64[word] ^ word_read;
                row_buffer_p64[word_id] = flips_mask;
              }
            }
          };

          nb::capsule owner(row_buffers, [](void *p) noexcept {
             _mm_free(p);
          });

          return nb::ndarray<nb::numpy, uint64_t, nb::ndim<2>>(
            (uint64_t*) row_buffers,
            {data_patterns.size(), row_size / sizeof(uint64_t)},
            owner
          );
  });

  m.def("init_row",  &init_row);
  m.def("init_rows", &init_rows);
  m.def("init_row_range", &init_row_range);

  m.def("read_row",  &read_row);
  m.def("read_rows", &read_rows);
  m.def("read_row_range", &read_row_range);

  m.def("act_row",   &act_row);
  m.def("act_rows",  &act_rows);
  m.def("refresh_all_rows",  &refresh_all_rows);

  m.def("rowclone",     &rowclone);
  m.def("tWR_test",     &tWR_test);
  m.def("frac",         &frac);
  // m.def("multirow_act", &multirow_act);

  m.def("singleside_hammer", &singleside_hammer);
  m.def("doubleside_hammer", &doubleside_hammer);

}