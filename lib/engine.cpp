#include <include/engine.h>

#define VSHADER_ROUTINE_SIZE 512
#define RASTERIZER_ROUTINE_SIZE 256
#define ZBUFFER_ROUTINE_SIZE 512
#define COLORS_ROUTINE_SIZE 16

#define pipeline_execute_tasks(_routine)				\
do {									\
	sync_tp.set_tasks(std::bind(&TrPipeline::_routine, this,	\
		std::placeholders::_1, std::placeholders::_2),		\
			task_buf.size());				\
	sync_tp.run();							\
	sync_tp.wait_completion();					\
	task_buf.clear();						\
} while (0)

void TrPipeline::VshaderRoutineProcess(int thread_id, int task_id)
{
	VshaderBuf &vshader_buf = vshader_buffers[thread_id];
	Task task = task_buf[task_id];

	Vec3 min_scr = cur_shader->vp_tr.min_scr;
	Vec3 max_scr = cur_shader->vp_tr.max_scr;
	min_scr.z = 0;
	max_scr.z = std::numeric_limits<float>::max();

	for (auto i = task.beg; i < task.end; ++i) {
		PrimBuf::value_type pr = (*cur_prim_buf)[i];
		VshaderBuf::value_type vs_pr;
		for (int n = 0; n < 3; ++n)
			vs_pr[n] = cur_shader->VShader(pr[n]);

		if (vs_pr[0].fs_vtx.pos.z < 0 &&
		    vs_pr[1].fs_vtx.pos.z < 0 &&
		    vs_pr[2].fs_vtx.pos.z < 0)
			 vshader_buf.push_back(vs_pr);
	}
}

void TrPipeline::VshaderRoutineCollect(int thread_id, int task_id)
{
	Task task = task_buf[task_id];
	VshaderBuf &vshader_buf = vshader_buffers[task.beg];

	for (uint32_t i = 0; i < vshader_buf.size(); ++i)
		(*cur_vshader_buf)[i + task.end] = vshader_buf[i];
}

void TrPipeline::VshaderStage(uint32_t model_id)
{
	ModelEntry &entry = model_buf[model_id];
	cur_prim_buf    = &entry.ptr->prim_buf;
	cur_vshader_buf = &entry.vshader_buf;
	cur_shader      = entry.shader;

	auto &prim_buf = entry.ptr->prim_buf;
	uint32_t task_size = VSHADER_ROUTINE_SIZE;
	uint32_t total_size = prim_buf.size();

	for (uint32_t offs = 0; offs < total_size; offs += task_size) {
		Task task;
		task.beg = offs;
		if (offs + task_size > total_size)
			task.end = task.beg + total_size - offs;
		else
			task.end = task.beg + task_size;
		task_buf.push_back(task);
	}
	pipeline_execute_tasks(VshaderRoutineProcess);

	uint32_t offs = 0;
	for (uint32_t i = 0; i < vshader_buffers.size(); ++i) {
		Task task;
		task.beg = i;
		task.end = offs;
		offs += vshader_buffers[i].size();
		task_buf.push_back(task);
	}
	(*cur_vshader_buf).resize(offs);
	pipeline_execute_tasks(VshaderRoutineCollect);

	for (auto &buf : vshader_buffers)
		buf.clear();
}

void TrPipeline::RasterizerRoutine(int thread_id, int task_id)
{
	uint32_t model_id = cur_model_id;
	VshaderBuf &vshader_buf = model_buf[model_id].vshader_buf;
	auto &rast_buf = rast_buffers[thread_id];
	Task task = task_buf[task_id];

	for (int pid = task.beg; pid < task.end; ++pid) {
		auto const &vs_pr = vshader_buf[pid];
		Vec3 scr_pos[3];
		for (int i = 0; i < 3; ++i)
			scr_pos[i] = ReinterpVec3(vs_pr[i].pos);
		rast.rasterize(scr_pos, pid, model_id, rast_buf);
	}
}

void TrPipeline::RasterizerStage(uint32_t model_id)
{
	cur_model_id = model_id;
	VshaderBuf &vshader_buf = model_buf[model_id].vshader_buf;

	uint32_t task_size = RASTERIZER_ROUTINE_SIZE;
	uint32_t total_size = vshader_buf.size();
	for (uint32_t offs = 0; offs < total_size; offs += task_size) {
		Task task;
		task.beg = offs;
		if (offs + task_size > total_size)
			task.end = task.beg + total_size - offs;
		else
			task.end = task.beg + task_size;
		task_buf.push_back(task);
	}
	pipeline_execute_tasks(RasterizerRoutine);
}

void TrPipeline::ZbufferRoutine(int thread_id, int task_id)
{
	Task task = task_buf[task_id];
	for (uint32_t tile = task.beg; tile < task.end; ++tile) {
		for (auto &rast_buf : rast_buffers) {
			for (auto const &rast_el : rast_buf[tile])
				zbuf.add_elem(tile, rast_el.offs, rast_el.fg);
			rast_buf[tile].clear();
		}
	}
}

void TrPipeline::ZbufferStage()
{
	uint32_t task_size = ZBUFFER_ROUTINE_SIZE;
	uint32_t total_size = rast_buffers[0].size();
	for (uint32_t offs = 0; offs < total_size; offs += task_size) {
		Task task;
		task.beg = offs;
		if (offs + task_size > total_size)
			task.end = task.beg + total_size - offs;
		else
			task.end = task.beg + task_size;
		task_buf.push_back(task);
	}
	pipeline_execute_tasks(ZbufferRoutine);
}

void TrPipeline::RenderFragments(uint32_t model_id)
{
	VshaderStage(model_id);
	RasterizerStage(model_id);
	ZbufferStage();
}

inline Fbuffer::Color RenderColor(TrInterpolator &interp,
		std::vector<TrPipeline::ModelEntry> &model_buf,
		TrZbuffer::elem const &e)
{
	ModelShader *shader = model_buf[e.model_id].shader;
	auto const &pr_vs = model_buf[e.model_id].vshader_buf[e.prim_id];
	Vertex pr_vtx[3] = { pr_vs[0].fs_vtx,
		pr_vs[1].fs_vtx, pr_vs[2].fs_vtx };
	Vertex vtx = interp.Interpolate(pr_vtx, e.bc);
	return shader->FShader(vtx);
}

void TrPipeline::RenderColorsRoutine(int thread_id, int task_id)
{
	Fbuffer::Color *cbuf = cur_color_buf;
	uint32_t cbuf_w = wnd.w;
	Task task = task_buf[task_id];

	for (uint32_t tile = task.beg; tile < task.end; ++tile) {
		for (uint32_t of = 0; of < zbuf.buf[0].size(); ++of) {
			decltype(zbuf)::elem &e = zbuf.buf[tile][of];
			if (e.depth != decltype(zbuf)::free_depth) {
				uint32_t x, y, ind;
				tl_tr.ToScr(tile, of, x, y);
				ind = x + y * cbuf_w;
				cbuf[ind] = RenderColor(interp, model_buf, e);
			}
			e.depth = decltype(zbuf)::free_depth;
		}
	}
}

void TrPipeline::RenderColors(Fbuffer::Color *cbuf)
{
	cur_color_buf = cbuf;

	uint32_t task_size = COLORS_ROUTINE_SIZE;
	uint32_t total_size = zbuf.buf.size();
	for (uint32_t offs = 0; offs < total_size; offs += task_size) {
		Task task;
		task.beg = offs;
		if (offs + task_size > total_size)
			task.end = task.beg + total_size - offs;
		else
			task.end = task.beg + task_size;
		task_buf.push_back(task);
	}
	pipeline_execute_tasks(RenderColorsRoutine);
}
