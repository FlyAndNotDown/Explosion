import { useEffect, useState } from 'react';
import { Button } from '@heroui/button';
import { Card, CardBody, CardHeader } from '@heroui/card';
import { Textarea } from '@heroui/input';
import { ScrollShadow } from '@heroui/scroll-shadow';
import { Select, SelectItem, addToast } from '@heroui/react';
import { QWebChannel } from '@/qwebchannel';

interface ComponentData {
  className: string;
  content: object;
}

interface SelectedEntityData {
  entity: number;
  comps: Array<ComponentData>;
}

export default function InspectorPage() {
  const [entityData, setEntityData] = useState<SelectedEntityData | null>(null);
  const [addableComponents, setAddableComponents] = useState(Array<string>);
  const [componentToAdd, setComponentToAdd] = useState('');
  const [editedContents, setEditedContents] = useState<Record<string, string>>({});
  const [connected, setConnected] = useState(false);

  useEffect(() => {
    new QWebChannel(window.qt.webChannelTransport, (channel: QWebChannel): void => {
      window.backend = channel.objects.backend;
      const refresh = (): void => {
        setEntityData(window.backend.selectedEntityData ?? null);
        setEditedContents({});
      };
      refresh();
      setAddableComponents(window.backend.addableComponents ?? []);
      window.backend.SelectedEntityDataChanged.connect(refresh);
      setConnected(true);
    });
  }, []);

  function contentText(comp: ComponentData): string {
    return editedContents[comp.className] ?? JSON.stringify(comp.content, null, 2);
  }

  function onEditContent(className: string, text: string): void {
    setEditedContents({ ...editedContents, [className]: text });
  }

  function onApply(comp: ComponentData): void {
    if (!entityData) {
      return;
    }
    try {
      const parsed = JSON.parse(contentText(comp));
      window.backend.UpdateComponent(entityData.entity, comp.className, parsed);
    } catch (e) {
      addToast({ title: 'Invalid JSON', description: String(e), color: 'danger' });
    }
  }

  function onRemove(comp: ComponentData): void {
    if (entityData) {
      window.backend.RemoveComponent(entityData.entity, comp.className);
    }
  }

  function onAddComponent(): void {
    if (entityData && componentToAdd) {
      window.backend.AddComponent(entityData.entity, componentToAdd);
    }
  }

  if (!entityData) {
    return (
      <div className='flex h-screen items-center justify-center bg-background text-default-400 text-sm'>
        {connected ? 'No entity selected' : 'Connecting...'}
      </div>
    );
  }

  return (
    <div className='flex flex-col h-screen p-2 gap-2 bg-background text-foreground'>
      <div className='flex gap-2 items-center'>
        <Select
          size='sm'
          aria-label='component to add'
          placeholder='Component...'
          selectedKeys={componentToAdd ? new Set([componentToAdd]) : new Set()}
          onSelectionChange={(keys) => setComponentToAdd(Array.from(keys as Set<string>)[0] ?? '')}
        >
          {addableComponents.map((name) => (
            <SelectItem key={name}>{name}</SelectItem>
          ))}
        </Select>
        <Button size='sm' color='primary' isDisabled={!componentToAdd} onPress={onAddComponent}>Add</Button>
      </div>
      <ScrollShadow className='flex-1 flex flex-col gap-2'>
        {(entityData.comps ?? []).map((comp) => (
          <Card key={comp.className} radius='sm'>
            <CardHeader className='flex justify-between items-center py-1'>
              <span className='text-sm font-semibold'>{comp.className}</span>
              <div className='flex gap-1'>
                <Button size='sm' variant='flat' color='primary' onPress={() => onApply(comp)}>Apply</Button>
                <Button size='sm' variant='flat' color='danger' onPress={() => onRemove(comp)}>Remove</Button>
              </div>
            </CardHeader>
            <CardBody className='py-1'>
              <Textarea
                aria-label={comp.className}
                minRows={2}
                maxRows={12}
                classNames={{ input: 'font-mono text-xs' }}
                value={contentText(comp)}
                onValueChange={(text) => onEditContent(comp.className, text)}
              />
            </CardBody>
          </Card>
        ))}
      </ScrollShadow>
    </div>
  );
}
