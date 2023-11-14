% udpGetTimes1.m
%
% testing program for getting data from 4 ch HARP 3B04 230307
% two channels at 200kHz/ch
% UDP 1 packet = 1252 bytes = 12 bytes time header + 1240 bytes data
% 1 datagram = 1 packet
%
% based on udpGetTimes1.m
%
% 230312 smw

clearvars

hsz = 12;           % packet head size (bytes)
nchpp = 2;          % number of channels per packet
sppch = 5*62;       % samples per packet per channel = 310
bps = 2;            % bytes per sample
dsz = sppch * nchpp * bps;         % packet data size (bytes) = 1240
psz = hsz + dsz;    % packet size (bytes) = 1252

blkinterval = 1550; % block/packet/datagram size microseconds = 1e6 * sppch/200e3

% Create a udpport object udpportObj that uses IPV4 and communicates in byte mode. The
% object is bound to the local host at "192.168.100.100" and the local port 50000 with
% port sharing disabled.
udpportObj = udpport("LocalHost","192.168.100.100","LocalPort",50000);

% need 100 bytes to get Open command through
% Write the data ['Open',zeros(1,96)] as a uint8 using the udpport object udpportObj to the device with
% address or host name "192.168.100.220" and port 50000.
write(udpportObj,['Open',zeros(1,96)],"uint8","192.168.100.220",50000);

fprintf('UDP from HARP, get data and plot\n')
pcnt = 0;
flag1 = 1;

h = figure(100);

while(1)
    % 1 packet = 1252 bytes = 12 bytes time header + 1240 bytes data
    % Read values of uint8 data using the udpport object udpportObj.
    data1 = read(udpportObj,psz,"uint8");
    pcnt = pcnt + 1;
    time1 = data1(1:6);
    usec = int32(swapbytes(typecast(uint8(data1(7:10)),'uint32')));  % microseconds (delta = 1550 usec)
    tstr = [char(datetime(data1(1:6),'Format','MM/dd/yy HH:mm:ss')),'.',num2str(usec)];

    data2 = swapbytes(typecast(uint8(data1(13:psz)),'uint16'));
    data3 = double(reshape(data2,nchpp,sppch)) - 2^15;

    if pcnt >= 100
        pcnt = 0;
    if flag1
        subplot(2,1,1), h1 = plot(data3(1,:));
        ht = title(tstr);
        subplot(2,1,2), h2 = plot(data3(2,:));
        flag1 = 0;
    else
        h1.YData = data3(1,:);
        h2.YData = data3(2,:);
        ht.String = tstr;
        drawnow
    end
    end
end

% close the connection
write(udpportObj,['Close',zeros(1,95)],"uint8","192.168.100.220",50000);

