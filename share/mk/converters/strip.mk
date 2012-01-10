# Copyright (c) 2011 Axel Gonzalez <loox@e-shell.net>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice unmodified, this list of conditions, and the following
#    disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

CONVERT_FROM_STRIP:=${NEW_CURRENT_PACKING_FILE_NAME}
CONVERT_TO_STRIP:=${CURRENT_PACKING_FILE_NAME}

# Let user override
CONVERTER_STRIP?=strip
#CONVERTER_STRIP_FLAGS?=

${CONVERT_TO_STRIP}:		${CONVERT_FROM_STRIP}
	@echo ========== Convert ${CONVERT_FROM_STRIP} to ${CONVERT_TO_STRIP} \
	    with ${CONVERTER_STRIP} ============
	rm -f ${CONVERT_TO_STRIP}
	PATH=${IMAGE_BUILD_PATHS} ${CONVERTER_STRIP} ${CONVERTER_STRIP_FLAGS} \
	    -o ${CONVERT_TO_STRIP} ${CONVERT_FROM_STRIP}

