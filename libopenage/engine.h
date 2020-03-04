// Copyright 2013-2019 the openage authors. See copying.md for legal info.

#pragma once

#include <cstdint>
#include <memory>
#include <SDL2/SDL.h>
#include <unordered_map>
#include <vector>
#include <QObject>

#include "log/log.h"
#include "log/file_logsink.h"
#include "audio/audio_manager.h"
// pxd: from libopenage.coord.coordmanager cimport CoordManager
#include "coord/coordmanager.h"
#include "coord/phys.h"
#include "coord/pixel.h"
// pxd: from libopenage.cvar cimport CVarManager
#include "cvar/cvar.h"
#include "gui/engine_info.h"
#include "handlers.h"
#include "job/job_manager.h"
// pxd: from libopenage.input.input_manager cimport InputManager
#include "input/input_manager.h"
#include "input/action.h"
#include "options.h"
#include "util/externalprofiler.h"
#include "util/fps.h"
#include "util/path.h"
#include "util/profiler.h"
#include "unit/selection.h"
#include "util/strings.h"
#include "screenshot.h"

namespace openage {

namespace gui {
class GUI;
}

namespace renderer {

class Font;
class FontManager;
class TextRenderer;
class Color;

} // openage::renderer

class DrawHandler;
class TickHandler;
class ResizeHandler;

class Generator;
class GameSpec;
class GameMain;

namespace gui {
class GuiItemLink;
} // openage::gui


/**
 * Qt signals for the engine.
 */
class EngineSignals : public QObject {
	Q_OBJECT

public:
signals:
	void global_binds_changed(const std::vector<std::string>& global_binds);
};


/**
 * main engine container.
 *
 * central foundation for everything the openage engine is capable of.
 *
 * pxd:
 *
 * cppclass Engine:
 *
 *     InputManager &get_input_manager() except +
 *     CVarManager &get_cvar_manager() except +
 *     CoordManager coord
 */
class Engine : public ResizeHandler, public options::OptionNode {
	friend class GameMain;

public:
	enum class mode {
		LEGACY,
		HEADLESS,
		FULL,
	};

	/**
	 * engine initialization method.
	 * starts the engine subsystems depending on the requested run mode.
	 */
	Engine(mode mode,
	       const util::Path &root_dir,
	       const std::shared_ptr<cvar::CVarManager> &cvar_manager);

	/**
	 * engine copy constructor.
	 */
	Engine(const Engine &copy) = delete;

	/**
	 * engine assignment operator.
	 */
	Engine &operator=(const Engine &copy) = delete;

	/**
	 * engine move constructor.
	 */
	Engine(Engine &&other) = delete;

	/**
	 * engine move operator.
	 */
	Engine &operator=(Engine &&other) = delete;

public:
	/**
	 * engine destructor, cleans up memory etc.
	 * deletes opengl context, the SDL window, and engine variables.
	 */
	virtual ~Engine();

	/**
	 * starts the engine loop.
	 */
	void run();

	/**
	 * enqueues the stop of the main loop.
	 */
	void stop();

	/**
	 * window resize handler function.
	 * recalculates opengl settings like viewport and projection matrices.
	 */
	bool on_resize(coord::viewport_delta new_size) override;

	/**
	 * Start a game with the given game generator.
	 */
	void start_game(const Generator &generator);

	/**
	 * Start a game with the given initialized game.
	 */
	void start_game(std::unique_ptr<GameMain> &&game);

	/**
	 * Stop the running game.
	 */
	void end_game();

	/**
	 * Draw the game version and the current FPS on screen.
	 */
	bool draw_debug_overlay();

	/**
	 * register a new input event handler, run for each input event.
	 */
	void register_input_action(InputHandler *handler);

	/**
	 * register a tick action, executed upon engine tick.
	 */
	void register_tick_action(TickHandler *handler);

	/**
	 * register a hud drawing handler, drawn in hud coordinates.
	 * order: 1 above, -1 below
	 */
	void register_drawhud_action(HudHandler *handler, int order = 1);

	/**
	 * register a draw handler, run in game coordinates.
	 */
	void register_draw_action(DrawHandler *handler);

	/**
	 * register a resize handler, run when the window size changes.
	 */
	void register_resize_action(ResizeHandler *handler);

	/**
	 * return the data directory where the engine was started from.
	 */
	const util::Path &get_root_dir();

	/**
	 * return currently running game or null if a game is not
	 * currently running
	 */
	GameMain *get_game();

	/**
	 * return this engine's job manager.
	 */
	job::JobManager *get_job_manager();

	/**
	 * return this engine's audio manager.
	 */
	audio::AudioManager &get_audio_manager();

	/**
	* return this engine's screenshot manager.
	*/
	ScreenshotManager &get_screenshot_manager();

	/**
	 * return this engine's action manager.
	 */
	input::ActionManager &get_action_manager();

	/**
	 * return this engine's cvar manager.
	 */
	cvar::CVarManager &get_cvar_manager();

	/**
	 * return this engine's keybind manager.
	 */
	input::InputManager &get_input_manager();

	/**
	 * return this engine's unit selection.
	 */
	UnitSelection *get_unit_selection();

	/**
	 * send keybindings help string to gui.
	 */
	void announce_global_binds();

	/**
	 * return this engine's text renderer.
	 */
	renderer::TextRenderer *get_text_renderer();

	/**
	 * return the number of nanoseconds that have passed
	 * for rendering the last frame.
	 *
	 * use that for fps-independent input actions.
	 */
	time_nsec_t lastframe_duration_nsec() const;

	/**
	 * render text at a position with the specified font size
	 */
	void render_text(coord::viewport position, size_t size, const renderer::Color &color, const char *format, ...) ATTRIBUTE_FORMAT(5, 6);

	/**
	 * move the phys3 camera incorporated in the engine
	 */
	void move_phys_camera(float x, float y, float amount=1.0);

	/**
	 * current engine state variable.
	 * to be set to false to stop the engine loop.
	 */
	bool running;

	/**
	 * FPS and game version are drawn when this is true.
	 */
	options::Var<bool> drawing_debug_overlay;

	/**
	* this allows to disable drawing of every registered hud.
	*/
	options::Var<bool> drawing_huds;

	/**
	 * profiler used by the engine
	 */
	util::ExternalProfiler external_profiler;

	/**
	 * this engine's coordinate manager.
	 */
	coord::CoordManager coord;

private:
	/**
	 * main engine loop function.
	 * this will be looped once per frame when the game is running.
	 *
	 * the loop invokes fps counting, SDL event handling,
	 * view translation, and calling the main draw_method.
	 */
	void loop();

	/**
	 * The engine root directory.
	 * Uses the openage fslike path abstraction that can mount paths into one.
	 *
	 * This means that this path does simulataneously lead to global assets,
	 * home-folder-assets, settings, and basically the whole filesystem access.
	 */
	util::Path root_dir;

	/**
	 * how many nanoseconds are in a frame (1e9 / fps_limit).
	 * 0 if there is no fps limit.
	 */
	time_nsec_t ns_per_frame;

	/**
	 * input event processor objects.
	 * called for each captured sdl input event.
	 */
	std::vector<InputHandler *> on_input_event;

	/**
	 * run on every engine tick, after input handling, before rendering
	 */
	std::vector<TickHandler *> on_engine_tick;

	/**
	 * run every time the game is being drawn,
	 * with the renderer set to the camgame system
	 */
	std::vector<DrawHandler *> on_drawgame;

	/**
	 * run every time the hud is being drawn,
	 * with the renderer set to the camhud system
	 */
	std::vector<HudHandler *> on_drawhud;

	/**
	 * list of handlers that are executed upon a resize event.
	 */
	std::vector<ResizeHandler *> on_resize_handler;

	/**
	 * the currently running game
	 */
	std::unique_ptr<GameMain> game;

	/**
	 * the engine's job manager, for asynchronous background task queuing.
	 */
	job::JobManager job_manager;

	/**
	 * This stores information to be accessible from the QML engine.
	 *
	 * Information in there (such as a pointer to the this engine)
	 * is then usable from within qml files, after some additional magic.
	 */
	gui::EngineQMLInfo qml_info;

	/**
	 * the frame counter measuring fps.
	 */
	util::FrameCounter fps_counter;

	/**
	 * the engine's screenshot manager.
	 */
	ScreenshotManager screenshot_manager;

	/**
	 * the engine's cvar manager.
	 */
	std::shared_ptr<cvar::CVarManager> cvar_manager;

	/**
	 * the engine's action manager.
	 */
	input::ActionManager action_manager;

	/**
	 * the engine's audio manager.
	 */
	audio::AudioManager audio_manager;

	/**
	 * the engine's keybind manager.
	 */
	input::InputManager input_manager;

	/**
	 * the engine's unit selection.
	 */
	std::unique_ptr<UnitSelection> unit_selection;

	/**
	 * the text fonts to be used for (can you believe it?) texts.
	 * maps fontsize -> font
	 */
	std::unordered_map<int, renderer::Font *> fonts;

	/**
	 * SDL window where everything is displayed within.
	 */
	SDL_Window *window;

	/**
	 * SDL OpenGL context, we'll only have one,
	 * but it would allow having multiple ones.
	 *
	 * This is actually a void * but sdl2 thought it was a good idea to
	 * name it like a differently.
	 */
	SDL_GLContext glcontext;

	/**
	 * the gui binding
	 */
	std::unique_ptr<gui::GUI> gui;

	/**
	 * the engines profiler
	 */
	util::Profiler profiler;

	/**
	 * ttf font loading manager
	 */
	std::unique_ptr<renderer::FontManager> font_manager;

	/**
	 * 2d text renderer
	 */
	std::unique_ptr<renderer::TextRenderer> text_renderer;

	/**
	 * Logsink to store messages to the filesystem.
	 */
	std::unique_ptr<log::FileSink> logsink_file;

public:
	/**
	 * Signal emitting capability for the engine.
	 */
	EngineSignals gui_signals;

	/**
	 * Link to the Qt GUI.
	 */
	gui::GuiItemLink *gui_link;
};

} // namespace openage
