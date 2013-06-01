#include <tchar.h>

#include <salviau/include/wtl/wtl_application.h>

#include <salviar/include/presenter_dev.h>
#include <salviar/include/shader.h>
#include <salviar/include/shaderregs.h>
#include <salviar/include/shader_object.h>
#include <salviar/include/renderer_impl.h>
#include <salviar/include/resource_manager.h>
#include <salviar/include/rasterizer.h>
#include <salviar/include/colors.h>

#include <salviax/include/resource/mesh/sa/mesh_io.h>
#include <salviax/include/resource/mesh/sa/mesh_io_obj.h>

#include <salviau/include/common/timer.h>
#include <salviau/include/common/presenter_utility.h>
#include <salviau/include/common/window.h>

#include <vector>

using namespace eflib;
using namespace salviar;
using namespace salviax;
using namespace salviax::resource;
using namespace salviau;

using boost::shared_ptr;
using boost::dynamic_pointer_cast;

using std::string;
using std::vector;
using std::cout;
using std::endl;

#define SASL_VERTEX_SHADER_ENABLED
#define SALVIA_PIXEL_SHADER_ENABLED

char const* vs_code =
"float4x4 wvpMatrix; \r\n"
"struct VSIn{ \r\n"
"	float4 pos: POSITION; \r\n"
"}; \r\n"
"struct VSOut{ \r\n"
"	float4 pos: sv_position; \r\n"
"}; \r\n"
"VSOut vs_main(VSIn in){ \r\n"
"	VSOut out; \r\n"
"	out.pos = mul(in.pos, wvpMatrix); \r\n"
"	return out; \r\n"
"} \r\n"
;

char const* ps_code =
"float4 color; \r\n"
"float4 ps_main(): COLOR \r\n"
"{ \r\n"
"	return color; \r\n"
"} \r\n"
;

class ps : public pixel_shader
{
public:
	vec4 color;
	ps()
	{
		declare_constant(_T("Color"),   color );
	}
	bool shader_prog(const vs_output& /*in*/, ps_output& out)
	{
		out.color[0] = color;
		return true;
	}
	virtual pixel_shader_ptr create_clone()
	{
		return pixel_shader_ptr(new ps(*this));
	}
	virtual void destroy_clone(pixel_shader_ptr& ps_clone)
	{
		ps_clone.reset();
	}
};

class bs : public blend_shader
{
public:
	bool shader_prog(size_t sample, backbuffer_pixel_out& inout, const ps_output& in)
	{
		color_rgba32f color(in.color[0]);
		inout.color( 0, sample, color_rgba32f(in.color[0]) );
		return true;
	}
};

class triangles: public quick_app{
public:
	triangles(): quick_app( create_wtl_application() ){}

protected:
	/** Event handlers @{ */
	virtual void on_create(){

		cout << "Creating window and device ..." << endl;

		string title( "Sample: Triangles" );
		impl->main_window()->set_title( title );
		
		boost::any view_handle_any = impl->main_window()->view_handle();
		present_dev = create_default_presenter( *boost::unsafe_any_cast<void*>(&view_handle_any) );

		renderer_parameters render_params = {0};
		render_params.backbuffer_format = pixel_format_color_bgra8;
		render_params.backbuffer_height = 512;
		render_params.backbuffer_width = 512;
		render_params.backbuffer_num_samples = 1;

		hsr = create_software_renderer(&render_params, present_dev);

		const framebuffer_ptr& fb = hsr->get_framebuffer();
		if (fb->get_num_samples() > 1){
			display_surf.reset(new surface(fb->get_width(),
				fb->get_height(), 1, fb->get_buffer_format()));
			pdsurf = display_surf.get();
		}
		else{
			display_surf.reset();
			pdsurf = fb->get_render_target(render_target_color, 0);
		}

		raster_desc rs_desc;
		rs_desc.cm = cull_none;
		rs_back.reset(new raster_state(rs_desc));

		cout << "Compiling vertex shader ... " << endl;
		vsc = compile( vs_code, lang_vertex_shader );
		hsr->set_vertex_shader_code( vsc );

		cout << "Compiling pixel shader ... " << endl;
		psc = compile( ps_code, lang_pixel_shader );
#ifdef SALVIA_PIXEL_SHADER_ENABLED
		hsr->set_pixel_shader_code(psc);
#endif

		mesh_ptr pmesh = create_planar(
			hsr.get(), 
			vec3(-30.0f, -1.0f, -30.0f), 
			vec3(15.0f, 0.0f, 1.0f), 
			vec3(0.0f, 0.0f, 1.0f),
			4, 60, false
			);
		meshes.push_back( pmesh );

		pmesh = create_planar(
			hsr.get(), 
			vec3(-5.0f, -5.0f, -30.0f), 
			vec3(0.0f, 4.0f, 0.0f), 
			vec3(0.0f, 0.0f, 1.0f),
			4, 60, false
			);

		meshes.push_back( pmesh );

		pmesh = create_box( hsr.get() );
		meshes.push_back( pmesh );

		num_frames = 0;
		accumulate_time = 0;
		fps = 0;

		pps.reset( new ps() );
		pbs.reset( new bs() );
	}
	/** @} */

	void on_draw(){
		present_dev->present(*pdsurf);
	}

	void on_idle(){
		// measure statistics
		++ num_frames;
		float elapsed_time = static_cast<float>(timer.elapsed());
		accumulate_time += elapsed_time;

		// check if new second
		if (accumulate_time > 1)
		{
			// new second - not 100% precise
			fps = num_frames / accumulate_time;

			accumulate_time = 0;
			num_frames  = 0;

			cout << fps << endl;
		}

		timer.restart();

		hsr->clear_color(0, color_rgba32f(0.05f, 0.05f, 0.2f, 1.0f));
		hsr->clear_depth(1.0f);

		static float t = 17.08067f;
		// t += elapsed_time / 50000.0f;
		float r = 2.0f * ( 1 + t );
		float x = r * cos(t);
		float y = r * sin(t);

		vec3 camera( x, 8.0f, y);
		vec4 camera_pos = vec4( camera, 1.0f );

		mat44 world(mat44::identity()), view, proj, wvp;
		mat_scale( world, 30.0f, 30.0f, 30.0f );
		mat_lookat(view, camera, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f));
		mat_perspective_fov(proj, static_cast<float>(HALF_PI), 1.0f, 0.1f, 1000.0f);

		static float ypos = 40.0f;
		ypos -= elapsed_time;
		if ( ypos < 1.0f ){
			ypos = 40.0f;
		}

		hsr->set_pixel_shader(pps);
		hsr->set_blend_shader(pbs);

		for(float i = 0 ; i < 1 ; i ++)
		{
			// mat_translate(world , -0.5f + i * 0.5f, 0, -0.5f + i * 0.5f);
			mat_mul(wvp, world, mat_mul(wvp, view, proj));

			hsr->set_rasterizer_state(rs_back);

			// C++ vertex shader and SASL vertex shader are all available.
			hsr->set_vertex_shader_code( vsc );
			hsr->set_vs_variable( "wvpMatrix", &wvp );

#ifdef SALVIA_PIXEL_SHADER_ENABLED
			hsr->set_pixel_shader_code(psc);
#endif
			vec4 color[3];
			color[0] = vec4( 0.3f, 0.7f, 0.3f, 1.0f );
			color[1] = vec4( 0.3f, 0.3f, 0.7f, 1.0f );
			color[2] = vec4( 0.7f, 0.3f, 0.3f, 1.0f );

			for( size_t i_mesh = 0; i_mesh < meshes.size(); ++i_mesh ){
				mesh_ptr cur_mesh = meshes[i_mesh];
				hsr->set_ps_variable( "color", &color[i_mesh] );
				pps->set_constant( _T("Color"), &color[i_mesh] );
				cur_mesh->render();
			}
		}

		if (hsr->get_framebuffer()->get_num_samples() > 1){
			hsr->get_framebuffer()->get_render_target(render_target_color, 0)->resolve(*display_surf);
		}

		impl->main_window()->refresh();
	}

protected:
	/** Properties @{ */
	device_ptr present_dev;
	renderer_ptr hsr;

	vector<mesh_ptr>			meshes;
	shared_ptr<shader_object> vsc;
	shared_ptr<shader_object> psc;

	vertex_shader_ptr	pvs;
	pixel_shader_ptr	pps;
	blend_shader_ptr	pbs;

	raster_state_ptr rs_back;

	surface_ptr display_surf;
	surface* pdsurf;

	uint32_t num_frames;
	float accumulate_time;
	float fps;

	timer timer;
	/** @} */
};

int main( int /*argc*/, TCHAR* /*argv*/[] ){
	triangles loader;
	return loader.run();
}