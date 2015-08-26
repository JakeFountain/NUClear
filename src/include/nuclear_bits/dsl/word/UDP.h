/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_DSL_WORD_UDP_H
#define NUCLEAR_DSL_WORD_UDP_H

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <net/if.h>
#include <cstring>

#include "nuclear_bits/PowerPlant.h"
#include "nuclear_bits/dsl/word/IO.h"
#include "nuclear_bits/util/generate_reaction.h"
#include "nuclear_bits/util/network/get_interfaces.h"

namespace NUClear {
    namespace dsl {
        namespace word {
            
            struct UDP {
                
                struct Packet {
                    /// If the packet is valid (it contains data)
                    bool valid;
                    /// The address that the packet is from/to
                    uint32_t address;
                    /// The data to be sent in the packet
                    std::vector<char> data;
                    
                    /// Our validator when returned for if we are a real packet
                    operator bool() const {
                        return valid;
                    }
                };
                
                template <typename DSL, typename TFunc>
                static inline std::tuple<threading::ReactionHandle, int> bind(Reactor& reactor, const std::string& label, TFunc&& callback, int port) {
                    
                    // Make our socket
                    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                    if(fd < 0) {
                        throw std::system_error(errno, std::system_category(), "We were unable to open the UDP socket");
                    }
                    
                    // The address we will be binding to
                    sockaddr_in address;
                    memset(&address, 0, sizeof(sockaddr_in));
                    address.sin_family = AF_INET;
                    address.sin_port = htons(port);
                    address.sin_addr.s_addr = htonl(INADDR_ANY);
                    
                    // Bind to the address, and if we fail throw an error
                    if(::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(sockaddr))) {
                        throw std::system_error(errno, std::system_category(), "We were unable to bind the UDP socket to the port");
                    }
                    
                    // Get the port we ended up listening on
                    socklen_t len = sizeof(sockaddr_in);
                    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&address), &len) == -1) {
                        throw std::system_error(errno, std::system_category(), "We were unable to get the port from the TCP socket");
                    }
                    port = ntohs(address.sin_port);
                    
                    // Generate a reaction for the IO system that closes on death
                    auto reaction = util::generate_reaction<DSL, IO>(reactor, label, std::forward<TFunc>(callback), [fd] (threading::Reaction&) {
                        ::close(fd);
                    });
                    threading::ReactionHandle handle(reaction.get());
                    
                    // Send our configuration out
                    reactor.powerplant.emit<emit::Direct>(std::make_unique<IOConfiguration>(IOConfiguration {
                        fd,
                        IO::READ,
                        std::move(reaction)
                    }));
                    
                    // Return our handles and our bound port
                    return std::make_tuple(handle, port);
                }
                
                template <typename DSL>
                static inline Packet get(threading::ReactionTask& r) {
                    
                    // Get our filedescriptor from the magic cache
                    auto event = IO::get<DSL>(r);
                    
                    // If our get is being run without an fd (something else triggered) then short circuit
                    if (event.fd == 0) {
                        Packet p;
                        p.address = INADDR_NONE;
                        p.valid = false;
                        return p;
                    }
                    
                    // Make a packet with 2k of storage (hopefully packets are smaller then this as most MTUs are around 1500)
                    Packet p;
                    p.address = INADDR_NONE;
                    p.valid = false;
                    p.data.resize(2048);
                    
                    // Make a socket address to store our sender information
                    sockaddr_in from;
                    socklen_t sSize = sizeof(sockaddr_in);
                    
                    ssize_t received = recvfrom(event.fd, p.data.data(), p.data.size(), 0, reinterpret_cast<sockaddr*>(&from), &sSize);
                    
                    // if no error
                    if(received > 0) {
                        p.valid = true;
                        p.address = ntohl(from.sin_addr.s_addr);
                        p.data.resize(received);
                    }
                    
                    return p;
                }
                
                struct Broadcast {
                    
                    template <typename DSL, typename TFunc>
                    static inline std::tuple<threading::ReactionHandle, int> bind(Reactor& reactor, const std::string& label, TFunc&& callback, int port) {
                       
                        // Our list of broadcast file descriptors
                        std::vector<int> fds;
                        
                        // Get all the network interfaces
                        auto interfaces = util::network::get_interfaces();
                        
                        std::vector<uint32_t> addresses;
                        
                        for(auto& iface : interfaces) {
                            // We receive on broadcast addresses and we don't want loopback or point to point
                            if(!iface.flags.loopback && !iface.flags.pointtopoint && iface.flags.broadcast) {
                                // Two broadcast ips that are the same are probably on the same network so ignore those
                                if(std::find(std::begin(addresses), std::end(addresses), iface.broadcast) == std::end(addresses)) {
                                    addresses.push_back(iface.broadcast);
                                }
                            }
                        }
                        
                        for(auto& ad : addresses) {
                            
                            // Make our socket
                            int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                            if(fd < 0) {
                                throw std::system_error(errno, std::system_category(), "We were unable to open the UDP socket");
                            }
                            
                            // The address we will be binding to (our broadcast address)
                            sockaddr_in address;
                            memset(&address, 0, sizeof(sockaddr_in));
                            address.sin_family = AF_INET;
                            address.sin_port = htons(port);
                            address.sin_addr.s_addr = htonl(ad);
                            
                            // We are a broadcast socket
                            int yes = true;
                            if(setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)) < 0) {
                                throw std::system_error(errno, std::system_category(), "We were unable to set the socket as broadcast");
                            }
                            if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
                                throw std::system_error(errno, std::system_category(), "We were unable to set reuse address on the socket");
                            }
                            
                            // Bind to the address, and if we fail throw an error
                            if(::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(sockaddr))) {
                                throw std::system_error(errno, std::system_category(), "We were unable to bind the UDP socket to the port");
                            }
                            
                            // Set the port variable to whatever was returned (so they all use the same port)
                            socklen_t len = sizeof(sockaddr_in);
                            if (::getsockname(fd, reinterpret_cast<sockaddr*>(&address), &len) == -1) {
                                throw std::system_error(errno, std::system_category(), "We were unable to get the port from the TCP socket");
                            }
                            port = ntohs(address.sin_port);
                            
                            fds.push_back(fd);
                        }
                        
                        // Generate a reaction for the IO system that closes on death
                        auto reaction = util::generate_reaction<DSL, IO>(reactor, label, std::forward<TFunc>(callback), [fds] (threading::Reaction&) {
                            // Close all the sockets
                            for(auto& fd : fds) {
                                ::close(fd);
                            }
                        });
                        std::shared_ptr<threading::Reaction> r(std::move(reaction));
                        threading::ReactionHandle handle(r.get());
                        
                        // Send our configuration out for each file descriptor (same reaction)
                        for(auto& fd : fds) {
                            reactor.powerplant.emit<emit::Direct>(std::make_unique<IOConfiguration>(IOConfiguration {
                                fd,
                                IO::READ,
                                r
                            }));
                        }
                        
                        // Return our handles
                        return std::make_tuple(handle, port);
                    }
                    
                    template <typename DSL>
                    static inline Packet get(threading::ReactionTask& r) {
                        return UDP::get<DSL>(r);
                    }

                };
                
                struct Multicast {
                    
                    template <typename DSL, typename TFunc>
                    static inline std::tuple<threading::ReactionHandle, int> bind(Reactor& reactor, const std::string& label, TFunc&& callback, std::string multicastGroup, int port) {
                        
                        // Our multicast group address
                        sockaddr_in address;
                        memset(&address, 0, sizeof(sockaddr_in));
                        address.sin_family = AF_INET;
                        address.sin_addr.s_addr = inet_addr(multicastGroup.c_str());
                        address.sin_port = htons(port);
                        
                        // Make our socket
                        int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                        if(fd < 0) {
                            throw std::system_error(errno, std::system_category(), "We were unable to open the UDP socket");
                        }
                        
                        int yes = true;
                        if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
                            throw std::system_error(errno, std::system_category(), "We were unable to set reuse address on the socket");
                        }
                        
                        // Bind to the address
                        if(::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(sockaddr))) {
                            throw std::system_error(errno, std::system_category(), "We were unable to bind the UDP socket to the port");
                        }
                        
                        // Store the port variable that was used
                        socklen_t len = sizeof(sockaddr_in);
                        if (::getsockname(fd, reinterpret_cast<sockaddr*>(&address), &len) == -1) {
                            throw std::system_error(errno, std::system_category(), "We were unable to get the port from the TCP socket");
                        }
                        port = ntohs(address.sin_port);
                        
                        // Get all the network interfaces that support multicast
                        std::vector<uint32_t> addresses;
                        for(auto& iface : util::network::get_interfaces()) {
                            // We receive on broadcast addresses and we don't want loopback or point to point
                            if(!iface.flags.loopback && !iface.flags.pointtopoint && iface.flags.multicast) {
                                // Two broadcast ips that are the same are probably on the same network so ignore those
                                if(std::find(std::begin(addresses), std::end(addresses), iface.broadcast) == std::end(addresses)) {
                                    addresses.push_back(iface.ip);
                                }
                            }
                        }
                        
                        for(auto& ad : addresses) {
                            
                            // Our multicast join request
                            ip_mreq mreq;
                            memset(&mreq, 0, sizeof(mreq));
                            mreq.imr_multiaddr.s_addr = address.sin_addr.s_addr;
                            mreq.imr_interface.s_addr = htonl(ad);
                            
                            // Join our multicast group but first leave because of stupid things
                            setsockopt(fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(ip_mreq));
                            if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(ip_mreq)) < 0) {
                                throw std::system_error(errno, std::system_category(), "There was an error while attemping to join the multicast group");
                            }
                        }
                        
                        // Generate a reaction for the IO system that closes on death
                        auto reaction = util::generate_reaction<DSL, IO>(reactor, label, std::forward<TFunc>(callback), [fd] (threading::Reaction&) {
                            // Close all the sockets
                            ::close(fd);
                        });
                        
                        std::shared_ptr<threading::Reaction> r(std::move(reaction));
                        threading::ReactionHandle handle(r.get());
                        
                        // Send our configuration out for each file descriptor (same reaction)
                        reactor.powerplant.emit<emit::Direct>(std::make_unique<IOConfiguration>(IOConfiguration {
                            fd,
                            IO::READ,
                            r
                        }));
                        
                        // Return our handles
                        return std::make_tuple(handle, port);
                    }
                    
                    template <typename DSL>
                    static inline Packet get(threading::ReactionTask& r) {
                        return UDP::get<DSL>(r);
                    }
                };
            };
        }
        
        namespace trait {
            template <>
            struct is_transient<word::UDP::Packet> : public std::true_type {};
        }
    }
}

#endif
