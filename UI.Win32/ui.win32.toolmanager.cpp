module ui.win32.toolmanager;

namespace ui::win32
{
	void ToolManager::set_active_tool(ITool* tool)
	{
		current = tool;
	}

	ITool* ToolManager::active_tool() const noexcept
	{
		return current;
	}

	void ToolManager::dispatch_pointer_down(const PointerEvent& e)
	{
		if (current) current->on_pointer_down(e);
	}

	void ToolManager::dispatch_pointer_move(const PointerEvent& e)
	{
		if (current) current->on_pointer_move(e);
	}

	void ToolManager::dispatch_pointer_up(const PointerEvent& e)
	{
		if (current) current->on_pointer_up(e);
	}

	void ToolManager::dispatch_key_down(const KeyEvent& e)
	{
		if (current) current->on_key_down(e);
	}

	void ToolManager::dispatch_key_up(const KeyEvent& e)
	{
		if (current) current->on_key_up(e);
	}

	void ToolManager::update()
	{
		if (current) current->update();
	}

	void ToolManager::render_overlays()
	{
		if (current) current->render_overlays();
	}
}
