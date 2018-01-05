#include "Node.h"
#include "Service.h"

bool Node::IsLocal() const
{
	return Service::Instance.GetLocalNode() == this;
}
