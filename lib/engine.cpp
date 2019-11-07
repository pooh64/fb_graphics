#include <include/engine.h>

void VshaderStage(ModelShader *shader,
		std::vector<std::array<ModelShader::vs_out, 3>> &vshader_buf,
		std::vector<std::array<Vertex, 3>> const &prim_buf)
{
	Vec3 min_scr = shader->vp_tr.min_scr;
	Vec3 max_scr = shader->vp_tr.max_scr;
	min_scr.z = 0;
	max_scr.z = std::numeric_limits<float>::max();

	for (auto const &pr : prim_buf) {
		std::array<ModelShader::vs_out, 3> vs_pr;
		for (int i = 0; i < 3; ++i)
			vs_pr[i] = shader->VShader(pr[i]);
#if 1
#if 0
		Vec3 min_r{std::min(vs_pr[0].pos.x, std::min(vs_pr[1].pos.x, vs_pr[2].pos.x)),
			   std::min(vs_pr[0].pos.y, std::min(vs_pr[1].pos.y, vs_pr[2].pos.y)),
			   std::min(vs_pr[0].pos.z, std::min(vs_pr[1].pos.z, vs_pr[2].pos.z))};
		Vec3 max_r{std::max(vs_pr[0].pos.x, std::max(vs_pr[1].pos.x, vs_pr[2].pos.x)),
			   std::max(vs_pr[0].pos.y, std::max(vs_pr[1].pos.y, vs_pr[2].pos.y)),
			   std::max(vs_pr[0].pos.z, std::max(vs_pr[1].pos.z, vs_pr[2].pos.z))};

		if (max_r > min_scr && max_scr > min_r)
			vshader_buf.push_back(vs_pr);

#else
		for (int i = 0; i < 3; ++i) {
			Vec3 pos = ReinterpVec3(vs_pr[i].pos);
			if (pos.x <= max_scr.x && pos.y <= max_scr.y && true &&
			    pos.x >= min_scr.x && pos.y >= min_scr.y && pos.z >= min_scr.z) {
				vshader_buf.push_back(vs_pr);
				break;
			}
			if (pos.z < min_scr.z)
				break;
			if (i == 2)
				vshader_buf.push_back(vs_pr);
		}

#endif
#else
		vshader_buf.push_back(vs_pr);
#endif
	}
}

void RasterizerStage(TrRasterizer &rast, uint32_t model_id,
		std::vector<std::vector<TrRasterizer::rz_out>> &rast_buf,
		std::vector<std::array<ModelShader::vs_out, 3>> &vshader_buf)
{
	//std::cout << "RasterizerStage: " << vshader_buf.size() << std::endl;
	for (int pid = 0; pid < vshader_buf.size(); ++pid) {
		auto const &vs_pr = vshader_buf[pid];
		Vec3 scr_pos[3];
		for (int i = 0; i < 3; ++i)
			scr_pos[i] = ReinterpVec3(vs_pr[i].pos);
		rast.rasterize(scr_pos, pid, model_id, rast_buf);
	}
}

void ZbufferStage(TrZbuffer &zbuf,
		std::vector<std::vector<TrRasterizer::rz_out>> &rast_buf)
{
	for (uint32_t y = 0; y < rast_buf.size(); ++y) {
		for (auto const &rast_el : rast_buf[y])
			zbuf.add_elem(rast_el.x, y, rast_el.fg);
	}

	for (auto &line : rast_buf)
		line.clear();
}

void TrPipeline::RenderToZbuf(uint32_t model_id)
{
	ModelEntry &entry = model_buf[model_id];
	shader = entry.shader;
	TrModel const &obj = *entry.ptr;

	VshaderStage(shader, entry.vshader_buf, obj.prim_buf);
	RasterizerStage(rast, model_id, rast_buf, entry.vshader_buf);
	ZbufferStage(zbuf, rast_buf);
}

inline Fbuffer::Color RenderFragment(TrInterpolator &interp,
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

void TrPipeline::RenderToCbuf(Fbuffer::Color *cbuf)
{
	uint32_t zbuf_w = zbuf.wnd.w;
	uint32_t zbuf_h = zbuf.wnd.h;
	for (uint32_t y = 0; y < zbuf_h; ++y) {
		uint32_t ind = y * zbuf_w;
		for (uint32_t x = 0; x < zbuf_w; ++x, ++ind) {
			decltype(zbuf)::elem e = zbuf.buf[ind];
			zbuf.buf[ind].depth = decltype(zbuf)::free_depth;
			if (e.depth != decltype(zbuf)::free_depth)
				cbuf[ind] = RenderFragment(interp,
						model_buf, e);
		}
	}
}
