#include "node.h"

namespace multiverso {

Node::Node() : rank(-1), role(-1), worker_id(-1), server_id(-1) {}

namespace node {
bool is_worker(int role) { return (role & 1) != 0; }
bool is_server(int role) { return (role & 2) != 0; }

}

}