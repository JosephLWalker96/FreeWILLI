%address = "192.168.1.24"; % use this for wifi
address = '192.168.7.2';
port = 1045;

udpObject = udpport("datagram", "IPV4");

% Send data to the remote host
for x = 0:0.1:2*pi
    y = sin(x);

    % Convert the data to bytes before sending
    dataToSend = randn(1, 100);  % Array of 100 doubles

    write(udpObject, dataToSend, address, port);

    pause(1);
end



