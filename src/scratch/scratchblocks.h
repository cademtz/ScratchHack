#pragma once
#include <vector>
#include "scratchopcodes.h"

class ScratchBlock
{
public:


	inline std::vector<ScratchBlock*>& Children() { return m_children; }

protected:
	ScratchBlock* m_next = 0;
	std::vector<ScratchBlock*> m_children;

private:

};