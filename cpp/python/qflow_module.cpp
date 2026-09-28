#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "qflow/indicator.hpp"
#include "qflow/instrument.hpp"
#include "qflow/portfolio.hpp"
#include "qflow/tick.hpp"
#include "qflow/version.hpp"

namespace py = pybind11;

PYBIND11_MODULE(qflow, m) {
  m.doc() = "QFlow Python research bindings (cold path)";
  m.def("version", &qflow::version, "Library version string");

  py::enum_<qflow::Side>(m, "Side")
      .value("Buy", qflow::Side::Buy)
      .value("Sell", qflow::Side::Sell);

  py::class_<qflow::Tick>(m, "Tick")
      .def(py::init<>())
      .def_readwrite("ts_ns", &qflow::Tick::ts_ns)
      .def_readwrite("price", &qflow::Tick::price)
      .def_readwrite("size", &qflow::Tick::size)
      .def_readwrite("instrument_id", &qflow::Tick::instrument_id)
      .def_readwrite("side", &qflow::Tick::side);

  m.def("to_ticks", &qflow::to_ticks, py::arg("price"), py::arg("tick_size"));
  m.def("to_price", &qflow::to_price, py::arg("ticks"), py::arg("tick_size"));

  py::class_<qflow::Sma>(m, "Sma")
      .def(py::init<std::size_t>(), py::arg("period"))
      .def("update", &qflow::Sma::update)
      .def("ready", &qflow::Sma::ready)
      .def("value", &qflow::Sma::value);

  py::class_<qflow::Ema>(m, "Ema")
      .def(py::init<std::size_t>(), py::arg("period"))
      .def("update", &qflow::Ema::update)
      .def("ready", &qflow::Ema::ready)
      .def("value", &qflow::Ema::value);

  py::class_<qflow::Portfolio>(m, "Portfolio")
      .def(py::init<qflow::MoneyCents>(), py::arg("initial_cash_cents"))
      .def("cash_cents", &qflow::Portfolio::cash_cents)
      .def("position_qty",
           [](const qflow::Portfolio& p, qflow::InstrumentId id) {
             return p.position(id).quantity;
           });
}
