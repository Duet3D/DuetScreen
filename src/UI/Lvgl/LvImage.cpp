#include "LvImage.h"
#include "Debug.h"

namespace UI
{
	LvImage::LvImage(const std::string& name, LvObj& parent)
		: LvImageGen(name, parent)
	{
		setInnerAlign(LV_IMAGE_ALIGN_CONTAIN);
	}
} // namespace UI
