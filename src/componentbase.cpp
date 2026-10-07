#include <tavoos/componentbase.h>

#include <tavoos/builder.h>
#include <tavoos/widget/widget.h>

namespace Tavoos {

void ComponentBase::runBuild() {
    auto* const self = dynamic_cast<Widget*>(this);
    Object* const previous = Builder::currentItem;
    Builder::currentItem = self;
    build();
    Builder::currentItem = previous;
}

}
