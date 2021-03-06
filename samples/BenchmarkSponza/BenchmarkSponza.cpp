#include <salviar/include/shader.h>
#include <salviar/include/shader_regs.h>
#include <salviar/include/shader_object.h>
#include <salviar/include/sync_renderer.h>
#include <salviar/include/resource_manager.h>
#include <salviar/include/rasterizer.h>
#include <salviar/include/colors.h>
#include <salviar/include/texture.h>

#include <salviax/include/resource/mesh/sa/mesh_io.h>
#include <salviax/include/resource/mesh/sa/mesh_io_obj.h>
#include <salviax/include/resource/mesh/sa/material.h>
#include <salviax/include/resource/texture/tex_io.h>

#include <salviau/include/common/timer.h>
#include <salviau/include/common/window.h>
#include <salviau/include/common/benchmark.h>

#include <eflib/include/diagnostics/profiler.h>

#include <vector>

#if defined( SALVIA_BUILD_WITH_DIRECTX )
#define PRESENTER_NAME "d3d9"
#else
#define PRESENTER_NAME "opengl"
#endif

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

char const* benchmark_vs_code =
"float4x4 wvpMatrix; \r\n"
"float4   eyePos; \r\n"
"float4	  lightPos; \r\n"
"struct VSIn{ \r\n"
"	float4 pos: POSITION; \r\n"
"	float4 norm: NORMAL; \r\n"
"	float4 tex: TEXCOORD0; \r\n"
"}; \r\n"
"struct VSOut{ \r\n"
"	float4 pos: sv_position; \r\n"
"	float4 tex: TEXCOORD; \r\n"
"	float4 norm: TEXCOORD1; \r\n"
"	float4 lightDir: TEXCOORD2; \r\n"
"	float4 eyeDir: TEXCOORD3; \r\n"
"}; \r\n"
"VSOut vs_main(VSIn in){ \r\n"
"	VSOut out; \r\n"
"	out.norm = in.norm; \r\n"
"	out.pos = mul(in.pos, wvpMatrix); \r\n"
"	out.lightDir = lightPos - in.pos; \r\n"
"	out.eyeDir = eyePos - in.pos; \r\n"
"	out.tex = in.tex; \r\n"
"	return out; \r\n"
"} \r\n"
;

class benchmark_vs : public cpp_vertex_shader
{
	mat44 wvp;
	vec4 light_pos, eye_pos;
public:
	benchmark_vs():wvp(mat44::identity()){
		declare_constant(_EFLIB_T("wvpMatrix"), wvp);
		declare_constant(_EFLIB_T("lightPos"),  light_pos);
		declare_constant(_EFLIB_T("eyePos"),    eye_pos);

		bind_semantic("POSITION", 0, 0);
		bind_semantic("TEXCOORD", 0, 1);
		bind_semantic("NORMAL",   0, 2);
	}

	benchmark_vs(const mat44& wvp):wvp(wvp){}
	void shader_prog(const vs_input& in, vs_output& out)
	{
		vec4 pos = in.attribute(0);
		transform(out.position(), pos, wvp);
		out.attribute(0) = in.attribute(1);
		out.attribute(1) = in.attribute(2);
		out.attribute(2) = light_pos - pos;
		out.attribute(3) = eye_pos - pos;
	}

	uint32_t num_output_attributes() const
	{
		return 4;
	}

	uint32_t output_attribute_modifiers(uint32_t) const
	{
		return salviar::vs_output::am_linear;
	}

    virtual cpp_shader_ptr clone()
	{
        typedef std::remove_pointer<decltype(this)>::type this_type;
		return cpp_shader_ptr(new this_type(*this));
	}

};

class benchmark_ps : public cpp_pixel_shader
{
	salviar::sampler_ptr sampler_;

	vec4 ambient;
	vec4 diffuse;
	vec4 specular;

	int shininess;
public:
	benchmark_ps()
	{
		declare_constant(_EFLIB_T("Ambient"),   ambient );
		declare_constant(_EFLIB_T("Diffuse"),   diffuse );
		declare_constant(_EFLIB_T("Specular"),  specular );
		declare_constant(_EFLIB_T("Shininess"), shininess );
        declare_sampler (_EFLIB_T("Sampler"),   sampler_);
	}

	bool shader_prog(const vs_output& in, ps_output& out)
	{
		vec4 diff_color = vec4(1.0f, 1.0f, 1.0f, 1.0f); // diffuse;

		if(sampler_)
		{
			diff_color = tex2d(*sampler_, 0).get_vec4();
		}

		vec3 norm( normalize3( in.attribute(1).xyz() ) );
		vec3 light_dir( normalize3( in.attribute(2).xyz() ) );
		vec3 eye_dir( normalize3( in.attribute(3).xyz() ) );

		float illum_diffuse  = clamp(dot_prod3(light_dir, norm), 0.0f, 1.0f);
		float illum_specular = clamp(dot_prod3(reflect3(light_dir, norm), eye_dir), 0.0f, 1.0f);

		out.color[0] = ambient * 0.01f + diff_color * illum_diffuse + specular * illum_specular;
		out.color[0] = diff_color * illum_diffuse;
		out.color[0][3] = 1.0f;

		return true;
	}

    virtual cpp_shader_ptr clone()
	{
        typedef std::remove_pointer<decltype(this)>::type this_type;
		return cpp_shader_ptr(new this_type(*this));
	}
};

class bs : public cpp_blend_shader
{
public:
	bool shader_prog(size_t sample, pixel_accessor& inout, const ps_output& in)
	{
		color_rgba32f color(in.color[0]);
		inout.color( 0, sample, color_rgba32f(in.color[0]) );
		return true;
	}

    virtual cpp_shader_ptr clone()
	{
        typedef std::remove_pointer<decltype(this)>::type this_type;
		return cpp_shader_ptr(new this_type(*this));
	}

};

class benchmark_sponza: public benchmark
{
public:
	benchmark_sponza()
		:benchmark("BenchmarkSponza")
	{
	}
	void initialize()
	{
		renderer_ = create_benchmark_renderer();

		color_format_ = pixel_format_color_bgra8;
		height_ = 512;
		width_ = 512;
		sample_count_ = 1;

        color_surface_ = renderer_->create_tex2d(width_, height_, sample_count_, color_format_)->subresource(0);
        ds_surface_ = renderer_->create_tex2d(width_, height_, sample_count_, pixel_format_color_rg32f)->subresource(0);
        if(sample_count_ == 1)
        {
            resolved_color_surface_ = color_surface_;
        }
        else
        {
            resolved_color_surface_ = renderer_->create_tex2d(width_, height_, 1, color_format_)->subresource(0);
        }
        renderer_->set_render_targets(1, &color_surface_, ds_surface_);

        viewport vp;
        vp.w = static_cast<float>(width_);
        vp.h = static_cast<float>(height_);
        vp.x = 0;
        vp.y = 0;
        vp.minz = 0.0f;
        vp.maxz = 1.0f;
        renderer_->set_viewport(vp);

		raster_desc rs_desc;
		rs_desc.cm = cull_back;
		rs_back.reset(new raster_state(rs_desc));

#ifdef SASL_VERTEX_SHADER_ENABLED
		profiling( "CompilingVS", [this]()
		{
			vx_shader_ = compile(benchmark_vs_code, lang_vertex_shader);
		});
#endif

		profiling("LoadingMesh", [this]()
		{
			mesh_ = create_mesh_from_obj( renderer_.get(), "../../resources/models/sponza/sponza.obj", false );
		});

		cpp_vs.reset( new ::benchmark_vs() );
		cpp_ps.reset( new ::benchmark_ps() );
		cpp_bs.reset( new ::bs() );
	}
	
	void save_frame(std::string const& file_name)
	{
		profiling("SaveFrame", [&, this]()
		{
			if (color_surface_ != resolved_color_surface_)
			{
				color_surface_->resolve(*resolved_color_surface_);
			}
			save_surface(renderer_.get(), resolved_color_surface_, to_tstring(file_name), pixel_format_color_bgra8);
		});
	}

	void run()
	{
		begin_bench();

		initialize();

		for(int i = 0; i < 60; ++i)
		{
			if ( (i + 1) % 6 == 0 )
			{
				cout << "Frame " << i + 1 << "/" << 60 << endl;
			}
			begin_frame();
			render_frame();
			end_frame();
		}
		
		save_frame("BenchmarkSponza_Frame.png");

		end_bench();

		save_results("BenchmarkSponza_Result.log");
	}

	void render_frame()
	{
		profiling("BackBufferClearing", [this]()
		{
			renderer_->clear_color(color_surface_, color_rgba32f(0.2f, 0.2f, 0.5f, 1.0f));
			renderer_->clear_depth_stencil(ds_surface_, clear_depth | clear_stencil, 1.0f, 0);
		});

		vec3 camera(-36.0f, 8.0f, 0.0f);
		vec4 camera_pos = vec4( camera, 1.0f );

		mat44 world(mat44::identity()), view, proj, wvp;
		mat_lookat(view, camera, vec3(40.0f, 15.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
		mat_perspective_fov(proj, static_cast<float>(HALF_PI), 1.0f, 0.1f, 1000.0f);

		vec4 lightPos( 0.0f, 40.0f, 0.0f, 1.0f );

		renderer_->set_pixel_shader(cpp_ps);
		renderer_->set_blend_shader(cpp_bs);

		mat_translate(world , -0.5f, 0, -0.5f);
		mat_mul(wvp, world, mat_mul(wvp, view, proj));

		renderer_->set_rasterizer_state(rs_back);

		// C++ vertex shader and SASL vertex shader are all available.
#ifdef SASL_VERTEX_SHADER_ENABLED
		renderer_->set_vertex_shader_code(vx_shader_);
#else
		cpp_vs->set_constant( _T("wvpMatrix"), &wvp );
		cpp_vs->set_constant( _T("eyePos"), &camera_pos );
		cpp_vs->set_constant( _T("lightPos"), &lightPos );
		renderer_->set_vertex_shader(cpp_vs);
#endif
		renderer_->set_vs_variable( "wvpMatrix", &wvp );

		renderer_->set_vs_variable( "eyePos", &camera_pos );
		renderer_->set_vs_variable( "lightPos", &lightPos );

		profiling("Rendering", [this]()
		{
			for( size_t i_mesh = 0; i_mesh < mesh_.size(); ++i_mesh )
			{
				mesh_ptr cur_mesh = mesh_[i_mesh];

				shared_ptr<obj_material> mtl
					= dynamic_pointer_cast<obj_material>( cur_mesh->get_attached() );

				cpp_ps->set_constant( _EFLIB_T("Ambient"),  &mtl->ambient );
				cpp_ps->set_constant( _EFLIB_T("Diffuse"),  &mtl->diffuse );
				cpp_ps->set_constant( _EFLIB_T("Specular"), &mtl->specular );
				cpp_ps->set_constant( _EFLIB_T("Shininess"),&mtl->ambient );

				sampler_desc desc;
				desc.min_filter = filter_linear;
				desc.mag_filter = filter_linear;
				desc.mip_filter = filter_point;
				desc.addr_mode_u = address_wrap;
				desc.addr_mode_v = address_wrap;
				desc.addr_mode_w = address_wrap;

				cpp_ps->set_sampler(_EFLIB_T("Sampler"), renderer_->create_sampler(desc, mtl->tex));

				cur_mesh->render();
			}
		});
	}

protected:
    pixel_format            color_format_;
    size_t                  width_;
    size_t                  height_;
    uint32_t                sample_count_;
    surface_ptr             color_surface_;
    surface_ptr             resolved_color_surface_;
    surface_ptr             ds_surface_;

	vector<mesh_ptr>		mesh_;
	shader_object_ptr 	    vx_shader_;

	cpp_vertex_shader_ptr	cpp_vs;
	cpp_pixel_shader_ptr	cpp_ps;
	cpp_blend_shader_ptr	cpp_bs;

	raster_state_ptr	    rs_back;
};

int main( int /*argc*/, std::_tchar* /*argv*/[] )
{
	benchmark_sponza bm;
	bm.run();
	return 0;
}
