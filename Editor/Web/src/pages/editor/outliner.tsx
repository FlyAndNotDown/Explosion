import { useEffect, useState } from 'react';
import { Button } from '@heroui/button';
import { Input } from '@heroui/input';
import { Listbox, ListboxItem } from '@heroui/listbox';
import { ScrollShadow } from '@heroui/scroll-shadow';
import { QWebChannel } from '@/qwebchannel';

interface EntityInfo {
  id: number;
  name: string;
}

export default function OutlinerPage() {
  const [entities, setEntities] = useState(Array<EntityInfo>);
  const [selectedEntity, setSelectedEntity] = useState(0);
  const [renameValue, setRenameValue] = useState('');
  const [connected, setConnected] = useState(false);

  useEffect(() => {
    new QWebChannel(window.qt.webChannelTransport, (channel: QWebChannel): void => {
      window.backend = channel.objects.backend;
      setEntities(window.backend.entities ?? []);
      setSelectedEntity(window.backend.selectedEntity ?? 0);
      window.backend.EntitiesChanged.connect(() => setEntities(window.backend.entities ?? []));
      window.backend.SelectionChanged.connect(() => setSelectedEntity(window.backend.selectedEntity ?? 0));
      setConnected(true);
    });
  }, []);

  function onSelect(id: number): void {
    window.backend.SelectEntity(id);
  }

  function onCreateEntity(): void {
    window.backend.CreateEntity('Entity');
  }

  function onDestroySelected(): void {
    if (selectedEntity !== 0) {
      window.backend.DestroyEntity(selectedEntity);
    }
  }

  function onRename(): void {
    if (selectedEntity !== 0 && renameValue) {
      window.backend.RenameEntity(selectedEntity, renameValue);
      setRenameValue('');
    }
  }

  return (
    <div className='flex flex-col h-screen p-2 gap-2 bg-background text-foreground'>
      <div className='flex gap-2'>
        <Button size='sm' color='primary' isDisabled={!connected} onPress={onCreateEntity}>Add Entity</Button>
        <Button size='sm' color='danger' variant='flat' isDisabled={!connected || selectedEntity === 0} onPress={onDestroySelected}>Delete</Button>
      </div>
      <ScrollShadow className='flex-1'>
        <Listbox
          aria-label='entities'
          selectionMode='single'
          selectedKeys={selectedEntity !== 0 ? new Set([String(selectedEntity)]) : new Set()}
          onAction={(key) => onSelect(parseInt(key as string))}
        >
          {entities.map((entity) => (
            <ListboxItem key={String(entity.id)} textValue={entity.name}>
              <span className='text-sm'>{entity.name}</span>
              <span className='text-xs text-default-400 ml-2'>#{entity.id}</span>
            </ListboxItem>
          ))}
        </Listbox>
      </ScrollShadow>
      <div className='flex gap-2'>
        <Input size='sm' placeholder='Rename selected...' value={renameValue} onValueChange={setRenameValue} />
        <Button size='sm' isDisabled={!connected || selectedEntity === 0 || !renameValue} onPress={onRename}>Rename</Button>
      </div>
    </div>
  );
}
