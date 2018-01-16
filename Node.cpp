#include "Node.h"
#include "Service.h"
#include <iostream>

using namespace std;

bool Node::IsLocal() const
{
	return Service::Instance.GetLocalNode() == this;
}

std::ostream& operator<<(std::ostream& stream, Node* node)
{
	stream << node->GetName() << " - " << node->GetAddressName() << ":" << ntohs(node->GetAddress().sin_port);
	return stream;
}
