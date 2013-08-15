/**
 * Copyright (C) 2013 Jake Woods <jake.f.woods@gmail.com>, Trent Houliston <trent@houliston.me>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "NUClear/Extensions/Networking.h"
#include "NUClear/NetworkMessage.pb.h"

#include <sstream>

namespace NUClear {
    namespace Extensions {
        Networking::Networking(PowerPlant* parent) : Reactor(parent),
        running(true),
        context(1),
        pub(context, ZMQ_PUB),
        termPub(context, ZMQ_PUB),
        sub(context, ZMQ_SUB) {

            // Get our PGM address
            std::string address = addressForName(parent->configuration.networkGroup,
                                                 parent->configuration.networkPort);

            std::cout << "Network bound to address " << address << std::endl;

            // Bind our publisher to this address
            pub.bind(address.c_str());

            // Create a secondary inprocess publisher used to terminate the thread
            termPub.bind("inproc://networkmaster-term");

            // Connect our subscriber to this address and subscribe to all messages
            sub.connect(address.c_str());
            sub.connect("inproc://networkmaster-term");
            sub.setsockopt(ZMQ_SUBSCRIBE, 0, 0);

            // Build a task
            Internal::ThreadWorker::ServiceTask task(std::bind(&Networking::run, this),
                                                     std::bind(&Networking::kill, this));

            powerPlant->addServiceTask(task);
        }

        void Networking::run() {

            while (running) {

                zmq::message_t message;
                sub.recv(&message);

                // If our message size is 0, then it is probably our termination message
                if(message.size() > 0) {

                    Serialization::NetworkMessage proto;
                    proto.ParseFromArray(message.data(), message.size());

                    // Get our hash
                    Networking::Hash type;
                    memcpy(type.data, proto.type().data(), Networking::Hash::SIZE);

                    // Find this type's deserializer (if it exists)
                    auto it = deserialize.find(type);
                    if(it != std::end(deserialize)) {
                        it->second(proto.source(), proto.payload());
                    }
                }
            }
        }

        void Networking::kill() {

            // Set our running status to false
            running = false;

            // Send a message to ensure that our block is released
            zmq::message_t message(0);
            termPub.send(message);
        }

        std::string Networking::addressForName(std::string name, unsigned port) {

            // Hash our string
            Networking::Hash hash = Networking::murmurHash3(name.c_str(), name.size());

            // Convert our hash into a multicast address
            uint32_t addr = 0xE0000200 + (hash.hash() % (0xEFFFFFFF - 0xE0000200));
            
            // Split our mulitcast address into a epgm address
            std::stringstream out;
            
            // Build our string
            out << "epgm://"
            << ((addr >> 24) & 0xFF) << "."
            << ((addr >> 16) & 0xFF) << "."
            << ((addr >>  8) & 0xFF) << "."
            << (addr & 0xFF) << ":"
            << port;
            
            return out.str();
        }
    }
}
