#include <rb2.h>

DataNode::~DataNode()
{
	data->TryDestruct();
}

