package com.daonvshu.protocol.codec.codec

import com.daonvshu.protocol.codec.ProtocolFlag
import com.daonvshu.protocol.codec.flagdata.ProtocolFlagDataContent
import com.daonvshu.protocol.codec.flagdata.ProtocolFlagDataEnd
import com.daonvshu.protocol.codec.flagdata.ProtocolFlagDataSize
import com.daonvshu.protocol.codec.flagdata.ProtocolFlagDataVerify
import com.daonvshu.protocol.codec.utils.ProtocolFlagData

abstract class ProtocolCodec {
    protected val flags = mutableListOf<ProtocolFlagData>()
    protected val flagMap = arrayOfNulls<ProtocolFlagData?>(ProtocolFlag.FlagMax.ordinal)

    open fun setFlags(flagList: List<ProtocolFlagData>) {
        flags.clear()
        flags.addAll(flagList)

        for (flag in flagList) {
            flagMap[flag.flagType.ordinal] = flag
        }

        val flagEnd = get<ProtocolFlagDataEnd>(ProtocolFlag.FlagEnd)
        flagEnd?.setDependentFlag(
            get<ProtocolFlagDataSize>(ProtocolFlag.FlagSize),
            get<ProtocolFlagDataContent>(ProtocolFlag.FlagContent),
            get<ProtocolFlagDataVerify>(ProtocolFlag.FlagVerify)
        )

        val flagContent = get<ProtocolFlagDataContent>(ProtocolFlag.FlagContent)
        flagContent?.setDependentFlag(
            get<ProtocolFlagDataSize>(ProtocolFlag.FlagSize),
            get<ProtocolFlagDataVerify>(ProtocolFlag.FlagVerify),
            get<ProtocolFlagDataEnd>(ProtocolFlag.FlagEnd)
        )
    }

    fun setVerifyFlags(flags: List<ProtocolFlagData?>) {
        val flagVerify = get<ProtocolFlagDataVerify>(ProtocolFlag.FlagVerify)
        flagVerify?.setVerifyFlags(flags)
    }

    protected inline fun <reified T : ProtocolFlagData> get(flagType: ProtocolFlag): T? {
       val value = flagMap[flagType.ordinal] ?: return null
       if (value !is T) {
           return null
       }
       return value
    }
}
