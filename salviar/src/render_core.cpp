#include <salviar/include/render_core.h>

#include <salviar/include/binary_modules.h>
#include <salviar/include/shaderregs.h>
#include <salviar/include/shaderregs_op.h>
#include <salviar/include/clipper.h>
#include <salviar/include/resource_manager.h>
#include <salviar/include/rasterizer.h>
#include <salviar/include/framebuffer.h>
#include <salviar/include/surface.h>
#include <salviar/include/vertex_cache.h>
#include <salviar/include/stream_assembler.h>
#include <salviar/include/shader_unit.h>
#include <salviar/include/input_layout.h>
#include <salviar/include/host.h>
#include <salviar/include/render_stages.h>
#include <salviar/include/renderer.h>
#include <salviar/include/shader_cbuffer_impl.h>
#include <salviar/include/shader_reflection.h>
#include <salviar/include/shader_object.h>

BEGIN_NS_SALVIAR();

using namespace eflib;
using boost::shared_ptr;

void render_core::update(render_state_ptr const& state)
{
    state_ = state;
}

result render_core::execute()
{
    switch(state_->cmd)
    {
    case command_id::draw:
    case command_id::draw_index:
        return draw();
    case command_id::clear_color:
        return clear_color();
    case command_id::clear_depth_stencil:
        return clear_depth_stencil();
    }

    return result::failed;
}

result render_core::draw()
{
	stages_.assembler->update(state_.get());
	stages_.ras->update(state_.get());
	stages_.vert_cache->update(state_.get());
	stages_.host->update(state_.get());
    stages_.backend->update(state_.get());
	apply_shader_cbuffer();

    stages_.vert_cache->transform_vertices();
    stages_.ras->draw();

	return result::ok;
}

render_core::render_core()
{
	// Create stages
	stages_.host		= modules::host::create_host();
	stages_.vert_cache	= create_default_vertex_cache();
	stages_.assembler	.reset( new stream_assembler() );
	stages_.ras			.reset( new rasterizer() );
	stages_.backend		.reset( new framebuffer() );

	stages_.vert_cache->initialize(&stages_);
	stages_.host->initialize(&stages_);
	stages_.ras->initialize(&stages_);
	stages_.backend->initialize(&stages_);
}

void render_core::apply_shader_cbuffer()
{
	if(state_->vs_proto)
	{
		for(auto const& variable: state_->vx_cbuffer->variables())
		{
			auto const& var_name = variable.first;
			auto const& var_data = variable.second;
			auto var_data_addr = state_->vx_cbuffer->data_pointer(var_data);

			sv_layout* layout = state_->vx_shader->get_reflection()->input_sv_layout(var_name);
			if(layout->agg_type == aggt_array)
			{
				state_->vs_proto->set_variable_pointer(var_name, var_data_addr, var_data.length);
			}
			else
			{
				state_->vs_proto->set_variable(var_name, var_data_addr);
			}
		}

		for(auto const& samp: state_->vx_cbuffer->samplers())
		{
			state_->vs_proto->set_sampler(samp.first, samp.second);
		}
	}

	if(state_->ps_proto)
	{
		for(auto const& variable: state_->px_cbuffer->variables())
		{
			auto const& var_name = variable.first;
			auto const& var_data = variable.second;
			auto var_data_addr = state_->px_cbuffer->data_pointer(var_data);
			state_->ps_proto->set_variable(var_name, var_data_addr);
		}

		for(auto const& samp: state_->px_cbuffer->samplers())
		{
			state_->ps_proto->set_sampler(samp.first, samp.second);
		}
	}
}

result render_core::clear_color()
{
    state_->clear_color_target->fill_texels(state_->clear_color);
    return result::ok;
}

result render_core::clear_depth_stencil()
{
    auto ds_color = color_rgba32f(
            state_->clear_z, *reinterpret_cast<float*>(&state_->clear_stencil), 0.0f, 0.0f
            );
    state_->clear_ds_target->fill_texels(ds_color);
    return result::ok;
}

END_NS_SALVIAR();
