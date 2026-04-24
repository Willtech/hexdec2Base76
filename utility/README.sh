#!/bin/ksh
echo "After successful \`make && make test && doas make install && doas makewhatis\`"
echo "1. The send folder already has a sample random.bin"
echo "2. run \`./send.sh\` prepares for transfer."
echo "3. run \`./transfer.sh\` simulates transfer."
echo "4. run \`./receive.sh\` decodes to original."
echo "5. Validate. You check it out."
echo "6. When you are ready run \`./cleanup.sh\`"
