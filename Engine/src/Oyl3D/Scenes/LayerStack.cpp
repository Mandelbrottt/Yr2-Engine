#include "oylpch.h"
#include "LayerStack.h"

namespace oyl
{
    LayerStack::LayerStack()
    {
    }

    LayerStack::~LayerStack()
    {
        for (Ref<Layer> layer : m_layers)
        {
            layer->onDetach();
            layer.reset();
        }
    }

    void LayerStack::pushLayer(Ref<Layer> layer)
    {
        layer->onAttach();

        m_layers.emplace(m_layers.begin() + m_layerInsertIndex, std::move(layer));
        m_layerInsertIndex++;

    }

    void LayerStack::pushOverlay(Ref<Layer> overlay)
    {
        overlay->onAttach();
        m_layers.emplace_back(std::move(overlay));
    }

    void LayerStack::popLayer(const Ref<Layer>& layer)
    {
        auto it = std::find(m_layers.begin(), m_layers.end(), layer);
        if (it != m_layers.end())
        {
            m_layers.erase(it);
            m_layerInsertIndex--;
            
            layer->onDetach();
        }
    }

    void LayerStack::popOverlay(const Ref<Layer>& overlay)
    {
        auto it = std::find(m_layers.begin(), m_layers.end(), overlay);
        if (it != m_layers.end())
        {
            m_layers.erase(it);
            
            overlay->onDetach();
        }
    }
}
