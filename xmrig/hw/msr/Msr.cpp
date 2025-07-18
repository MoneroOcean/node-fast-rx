/* XMRig
 * Copyright (c) 2018-2021 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2021 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "hw/msr/Msr.h"
#include "base/io/log/Log.h"


namespace xmrig {


static std::weak_ptr<Msr> instance;


} // namespace xmrig

std::shared_ptr<xmrig::Msr> xmrig::Msr::get()
{
    auto msr = instance.lock();
    if (!msr) {
        msr      = std::make_shared<Msr>();
        instance = msr;
    }

    if (msr->isAvailable()) {
        return msr;
    }

    return {};
}


bool xmrig::Msr::write(uint32_t reg, uint64_t value, int32_t cpu, uint64_t mask, bool verbose)
{
    if (mask != MsrItem::kNoMask) {
        uint64_t old_value = 0;
        if (rdmsr(reg, cpu, old_value)) {
            value = MsrItem::maskedValue(old_value, value, mask);
        }
    }

    const bool result = wrmsr(reg, value, cpu);
    return result;
}


xmrig::MsrItem xmrig::Msr::read(uint32_t reg, int32_t cpu, bool verbose) const
{
    uint64_t value = 0;
    if (rdmsr(reg, cpu, value)) {
        return { reg, value };
    }
    return {};
}
