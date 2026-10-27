#pragma once

#include <tavoos/events/event.h>
#include <tavoos/export.hpp>
#include <tavoos/types.h>

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace Tavoos {

class Widget;

using DragData = std::map<std::string, std::string>;

class TAVOOS_EXPORT DropEvent : public Event {
public:
    DropEvent(EventType type, Widget* source, std::vector<std::string> keys, DragData data, std::vector<std::string> urls)
        : Event{type}, m_source{source}, m_keys{std::move(keys)}, m_data{std::move(data)}, m_urls{std::move(urls)} {
        accept();
    }

    float x() const { return m_x; }
    float y() const { return m_y; }
    Widget* source() const { return m_source; }
    const std::vector<std::string>& keys() const { return m_keys; }
    const DragData& data() const { return m_data; }
    const std::vector<std::string>& urls() const { return m_urls; }

    void setPosition(const Point& point) { m_x = point.x; m_y = point.y; }

private:
    float m_x{0.0f};
    float m_y{0.0f};
    Widget* m_source;
    std::vector<std::string> m_keys;
    DragData m_data;
    std::vector<std::string> m_urls;
};

}
