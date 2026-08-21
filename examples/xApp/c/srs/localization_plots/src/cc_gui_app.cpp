//  First SRS client
//  adapted from the example:Weather update client in C++
//  Connects SUB socket to tcp://localhost:5556
//  Collects weather updates and finds avg temp in zipcode
//

#include <zmq.hpp>

#include <atomic>
#include <cmath>
#include <iostream>
#include <thread>
#include <vector>

#include "cc_gui_app.h"
#include "srs_schema_generated.h"

int main(int argc, char* argv[])
{
  ChannelApp app("SRS Plots");

  std::atomic<bool> running{true};

  std::thread receiver([&] {
    zmq::context_t context{1};
    zmq::socket_t subscriber{context, zmq::socket_type::sub};

    subscriber.set(zmq::sockopt::subscribe, "");
    subscriber.set(zmq::sockopt::rcvtimeo, 100);
    subscriber.connect("tcp://localhost:5556");

    std::cout << "Collecting updates from SRS xApp...\n";

    while (running) {
      zmq::message_t message;

      const auto result = subscriber.recv(message, zmq::recv_flags::none);

      if (!result) {
        continue; // timeout
      }

      flatbuffers::Verifier verifier(static_cast<const uint8_t*>(message.data()), message.size());

      if (!SRSPlots::VerifySRSDataBuffer(verifier)) {
        std::cerr << "Invalid FlatBuffer\n";
        continue;
      }

      const auto* srs = SRSPlots::GetSRSData(message.data());

      const size_t num_antennas = srs->num_antennas();
      const size_t num_prgs = srs->num_prgs();
      const auto* amp2 = srs->srs_cfr_amp2();

      if (amp2 == nullptr || amp2->size() != num_antennas * num_prgs) {
        std::cerr << "Invalid CFR dimensions\n";
        continue;
      }

      std::vector<std::vector<float>> cfr(num_antennas, std::vector<float>(num_prgs));

      for (size_t antenna = 0; antenna < num_antennas; ++antenna) {
        for (size_t prg = 0; prg < num_prgs; ++prg) {
          const size_t index = antenna * num_prgs + prg;

          cfr[antenna][prg] = std::sqrt(static_cast<float>(amp2->Get(index)));
        }
      }

      app.UpdateCFR(cfr);
    }
  });

  // GUI normally needs to run on the main thread.
  app.Run();

  running = false;
  receiver.join();

  return 0;
}
