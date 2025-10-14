#include "BiosAffine.hpp"

#include <cmath>

namespace GBA::hle::affine {
	static bool ObjAffineSet(memory::Bus* bus, cpu::CPUContext& ctx, bool& branch) {
		auto& regs = ctx.m_regs;
		auto src_ptr = regs.GetReg(0);
		auto dst_ptr = regs.GetReg(1);
		auto count   = regs.GetReg(2);
		auto stride  = regs.GetReg(3);

		bus->m_time.access = memory::Access::NonSeq;

		while (count--) {
			auto sx = i16(bus->ReadFast<u16>(src_ptr + 0)) / 256.f;
			bus->m_time.access = memory::Access::Seq;
			auto sy = i16(bus->ReadFast<u16>(src_ptr + 2)) / 256.f;
			auto theta = (bus->ReadFast<u16>(src_ptr + 4) >> 8) / 128.f * 3.14f;
			src_ptr += 8;

			float a{}, b{}, c{}, d{};

			a = d = cosf(theta);
			b = c = sinf(theta);

			a *= sx;
			b *= -sx;
			c *= sy;
			d *= sy;

			bus->m_time.access = memory::Access::NonSeq;

			bus->WriteFast<i16>(dst_ptr + stride * 0, i16(a * 256));
			bus->m_time.access = memory::Access::Seq;
			bus->WriteFast<i16>(dst_ptr + stride * 1, i16(b * 256));
			bus->WriteFast<i16>(dst_ptr + stride * 2, i16(c * 256));
			bus->WriteFast<i16>(dst_ptr + stride * 3, i16(d * 256));
			dst_ptr += stride * 4;
		}

		return true;
	}

	void RegisterAffine(FunctionTable& table) {
		RegisterFunction(table, 0x0F, ObjAffineSet);
	}
}