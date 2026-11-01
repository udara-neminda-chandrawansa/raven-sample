/*
 * raven_sample: a small program on top of Raven.
 *
 * The interface is assets/main.xml, loaded when the program starts. The code
 * only binds the actions the XML names (OnClicked="...", OnChanged="...",
 * OnCommitted="...") and finds the widgets it needs by their Name.
 */
#include <Raven.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define change_dir _chdir
#else
#include <unistd.h>
#define change_dir chdir
#endif

static Rid g_window = R_INVALID_ID;
static int g_clicks = 0;

static int file_exists(const char* path) {
	FILE* file = fopen(path, "rb");
	if (file) fclose(file);
	return file != NULL;
}

/* Full path of the running program, or an empty string if it is not known. */
static void executable_folder(char* folder, size_t size) {
	folder[0] = '\0';
#ifdef _WIN32
	{
		char* path = NULL;
		if (_get_pgmptr(&path) != 0 || !path) return;
		snprintf(folder, size, "%s", path);
	}
#else
	{
		ssize_t length = readlink("/proc/self/exe", folder, size - 1);
		if (length <= 0) return;
		folder[length] = '\0';
	}
#endif
	/* keep the folder: cut the file name */
	{
		char* slash = strrchr(folder, '/');
		char* backslash = strrchr(folder, '\\');
		if (backslash && (!slash || backslash > slash)) slash = backslash;
		if (slash) *slash = '\0';
		else folder[0] = '\0';
	}
}

/*
 * main.xml and strings.csv are read from the working folder. During
 * development that is assets/ (VS Code starts the program there). Once
 * installed, the program may be started from anywhere, so look for them next
 * to the executable:
 *   <exe folder>/assets              a Windows install or a zip
 *   <exe folder>/../share/raven-sample   a Linux install (/usr/bin + /usr/share)
 */
static void enter_assets_folder(void) {
	static const char* const relative[] = { "assets", "../share/raven-sample" };
	char folder[1024], candidate[1200];
	size_t i;

	if (file_exists("main.xml")) return;
	executable_folder(folder, sizeof(folder));
	if (!folder[0]) return;
	for (i = 0; i < sizeof(relative) / sizeof(relative[0]); i++) {
		snprintf(candidate, sizeof(candidate), "%s/%s/main.xml", folder, relative[i]);
		if (file_exists(candidate)) {
			snprintf(candidate, sizeof(candidate), "%s/%s", folder, relative[i]);
			if (change_dir(candidate) == 0) return;
		}
	}
}

static void error_callback(RError code, RSeverity severity, const char* detail) {
	fprintf(stderr, "[Raven] %s (%s): %s\n", Rerror_to_string(code), Rseverity_to_string(severity), detail);
}

/* OnClicked="app.count" */
static void on_count(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(void)user_data;
	g_clicks++;
	Rlabel_set_text(Rwidget_find(g_window, "counter"), Rtr_plural("clicks", g_clicks));
}

/* OnClicked="app.reset" */
static void on_reset(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(void)user_data;
	g_clicks = 0;
	Rlabel_set_text(Rwidget_find(g_window, "counter"), Rtr("sample.none"));
}

/* OnCommitted="app.greet": Enter pressed in the TextInput */
static void on_greet(Rid sender, const char* action, void* user_data) {
	char text[256];
	const char* name = Rtext_input_get_text(sender);
	(void)action;
	(void)user_data;
	if (name && name[0]) {
		snprintf(text, sizeof(text), "%s, %s!", Rtr("sample.hello"), name);
	} else {
		snprintf(text, sizeof(text), "%s", Rtr("sample.ask_name"));
	}
	Rlabel_set_text(Rwidget_find(g_window, "greeting"), text);
}

/* OnChanged="app.volume": the slider moved */
static void on_volume(Rid sender, const char* action, void* user_data) {
	float value = Rslider_get_value(sender); /* 0..100 */
	char text[32];
	(void)action;
	(void)user_data;
	Rprogress_bar_set_value(Rwidget_find(g_window, "meter"), value / 100.0f); /* 0..1 */
	snprintf(text, sizeof(text), "%d%%", (int)(value + 0.5f));
	Rlabel_set_text(Rwidget_find(g_window, "meter_text"), text);
}

/* OnClicked="app.quit" */
static void on_quit(Rid sender, const char* action, void* user_data) {
	(void)sender;
	(void)action;
	(void)user_data;
	Rquit(0);
}

int main(void) {
	Rerror_set_callback(error_callback);
	enter_assets_folder();

	if (!Rinit(R_BACKEND_VULKAN)) {
		fprintf(stderr, "Cannot start Raven. Is a Vulkan driver installed?\n");
		return 1;
	}

	/* translations: assets/strings.csv, in the language of the system */
	Rlocale_load_csv("strings.csv");
	Rlocale_set_language(Rlocale_get_system_language());

	Raction_bind("app.count", on_count, NULL);
	Raction_bind("app.reset", on_reset, NULL);
	Raction_bind("app.greet", on_greet, NULL);
	Raction_bind("app.volume", on_volume, NULL);
	Raction_bind("app.quit", on_quit, NULL);

	g_window = Rwindow_create_from_xml_file("main.xml");
	if (g_window == R_INVALID_ID) {
		fprintf(stderr, "Cannot load main.xml (the program must run in the assets/ folder).\n");
		Rshutdown();
		return 1;
	}

	/* the loop is the application's: Rdraw_frame draws one frame per call */
	while (Rdraw_frame()) {
	}

	Rshutdown();
	return 0;
}
